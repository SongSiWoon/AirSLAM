#!/usr/bin/env python3
import sys
import onnx
from onnx import helper as h, checker as ch, TensorProto
from onnx import numpy_helper as nph
import numpy as np
from collections import OrderedDict


def make_param_dictionary(initializer):
    params = OrderedDict()
    for tensor in initializer:
        params[tensor.name] = tensor
    return params


def convert_params_to_int32(params_dict):
    converted = []
    for name, tensor in params_dict.items():
        if tensor.data_type == TensorProto.INT64:
            arr = nph.to_array(tensor).astype(np.int32)
            new_tensor = nph.from_array(arr, name=tensor.name)
            converted.append(new_tensor)
        else:
            converted.append(tensor)
    return converted


def convert_constant_nodes_to_int32(nodes):
    new_nodes = []
    for node in nodes:
        if node.op_type == "ConstantOfShape":
            for attr in node.attribute:
                if attr.name == "value" and attr.t.data_type == TensorProto.INT64:
                    arr = nph.to_array(attr.t).astype(np.int32)
                    new_t = nph.from_array(arr, name=attr.t.name)
                    attr.t.data_type = TensorProto.INT32
                    attr.t.ClearField("raw_data")
                    attr.t.raw_data = new_t.raw_data
                    attr.t.ClearField("dims")
                    attr.t.dims.extend(new_t.dims)
        if node.op_type == "Cast":
            for attr in node.attribute:
                if attr.name == "to" and attr.i == TensorProto.INT64:
                    attr.i = TensorProto.INT32
        if node.op_type == "Constant" and node.attribute:
            t = node.attribute[0].t
            if t.data_type == TensorProto.INT64:
                arr = nph.to_array(t).astype(np.int32)
                new_t = nph.from_array(arr, name=t.name)
                new_const = h.make_node(
                    "Constant",
                    inputs=[], outputs=node.output,
                    name=node.name, value=new_t
                )
                new_nodes.append(new_const)
                continue
        new_nodes.append(node)
    return new_nodes


def patch_shape_and_gather_with_cast(nodes):
    patched = []
    replace_map = {}
    # Insert Cast after each Shape
    for node in nodes:
        patched.append(node)
        if node.op_type == "Shape":
            orig_out = node.output[0]
            cast_out = orig_out + "_int32"
            cast_node = h.make_node(
                "Cast",
                inputs=[orig_out], outputs=[cast_out],
                name=node.name + "_CastToInt32", to=TensorProto.INT32
            )
            patched.append(cast_node)
            replace_map[orig_out] = cast_out

    # Redirect and insert Cast after relevant Gather nodes
    final_nodes = []
    for node in patched:
        if node.op_type == "Cast" and node.name.endswith("_CastToInt32"):
            final_nodes.append(node)
            continue
        node.input[:] = [replace_map.get(inp, inp) for inp in node.input]
        final_nodes.append(node)
        if node.op_type == "Gather" and node.input and node.input[0] in replace_map.values():
            orig_out = node.output[0]
            cast_out = orig_out + "_int32"
            cast_node = h.make_node(
                "Cast",
                inputs=[orig_out], outputs=[cast_out],
                name=node.name + "_CastToInt32", to=TensorProto.INT32
            )
            final_nodes.append(cast_node)
            replace_map[orig_out] = cast_out
    return final_nodes


def patch_floor_inputs(nodes):
    # Ensure Floor nodes receive FLOAT input
    new_nodes = []
    for node in nodes:
        if node.op_type == "Floor":
            inp = node.input[0]
            if inp.endswith("_int32"):
                float_inp = inp.replace("_int32", "_fp32")
                cast_node = h.make_node(
                    "Cast",
                    inputs=[inp], outputs=[float_inp],
                    name=node.name + "_CastToFloat", to=TensorProto.FLOAT
                )
                new_nodes.append(cast_node)
                node.input[0] = float_inp
        new_nodes.append(node)
    return new_nodes


def convert_and_patch(model_path, out_path):
    print(f"Loading model: {model_path}")
    model = onnx.load(model_path)
    ch.check_model(model)
    graph = model.graph

    print("Patching Shape and Gather with Cast to INT32...")
    nodes1 = patch_shape_and_gather_with_cast(list(graph.node))

    print("Converting INT64 initializers to INT32...")
    conv_init = convert_params_to_int32(make_param_dictionary(graph.initializer))

    print("Converting ConstantOfShape, Cast, Constant to INT32...")
    nodes2 = convert_constant_nodes_to_int32(nodes1)

    print("Patching Floor nodes to accept FLOAT...")
    nodes3 = patch_floor_inputs(nodes2)

    print("Rebuilding graph...")
    new_graph = h.make_graph(
        nodes3,
        graph.name + "_int32",
        graph.input,
        graph.output,
        initializer=conv_init
    )
    new_model = h.make_model(new_graph, producer_name="onnx-int32-converter")
    new_model.opset_import[0].version = model.opset_import[0].version
    ch.check_model(new_model)

    print(f"Saving converted model to: {out_path}")
    onnx.save_model(new_model, out_path)
    print("Done.")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <input.onnx> <output.onnx>")
        sys.exit(1)
    convert_and_patch(sys.argv[1], sys.argv[2])
