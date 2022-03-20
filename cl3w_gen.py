#!/usr/bin/env python

# This program is free software. It comes without any warranty, to the extent
# permitted by applicable law. You can redistribute it and/or modify it under
# the terms of the Do What The Fuck You Want To Public License, Version 2, as
# published by Sam Hocevar. See http://www.wtfpl.net/ for more details.

from typing import List, Tuple

import argparse
import os
import io
import fnmatch
from xml.dom.minidom import parseString
from urllib import request

SRC = """
 * This file was generated with cl3w_gen.py, part of cl3w
 * (hosted at https://github.com/cloudhan/cl3w)
"""

WTFPL = """
 * This program is free software. It comes without any warranty, to the extent
 * permitted by applicable law. You can redistribute it and/or modify it under
 * the terms of the Do What The Fuck You Want To Public License, Version 2, as
 * published by Sam Hocevar. See http://www.wtfpl.net/ for more details.
"""

HEADER = "/*" + SRC + " *" + WTFPL + "*/\n"
NO_LICENSE_HEADER = "/*" + SRC + "*/\n"

cl_stds = ["1.0", "1.1", "1.2", "2.0", "2.1", "2.2", "3.0"]

parser = argparse.ArgumentParser(description='cl3w generator script')
parser.add_argument("--root", type=str, default=os.path.dirname(__file__), help="output root directory")
parser.add_argument("--cl_xml", type=str,
                    default="https://github.com/KhronosGroup/OpenCL-Docs/raw/main/xml/cl.xml")
parser.add_argument("--indent", default=4, choices=["\t", 2, 4, 8])
parser.add_argument("--cl_std", type=str, default="1.2", choices=cl_stds)
parser.add_argument("--cl_ext", type=str, default=None, help="a file of a list of ext names, wildcard supported, # for comment")
parser.add_argument("--no_header", action="store_true")
parser.add_argument("--no_license", action="store_true")
args = parser.parse_args()

indent_unit = args.indent if isinstance(args.indent, str) else int(args.indent) * " "


def indent(level):
    return indent_unit * level


def download(url):
    print('Downloading {0}'.format(url))
    return request.urlopen(url).read()


def get_supported_func_names(dom, cl_std: str, cl_exts: List[str]):
    names = []
    supported_stds = set(cl_stds[:cl_stds.index(args.cl_std) + 1])
    for feature in dom.getElementsByTagName("feature"):
        if feature.getAttribute("number") in supported_stds:
            for command in feature.getElementsByTagName("command"):
                names.append(command.getAttribute("name"))

    for available_extension in dom.getElementsByTagName("extensions")[0].getElementsByTagName("extension"):
        for enabled_ext in cl_exts:
            if fnmatch.fnmatch(available_extension.getAttribute("name"), enabled_ext):
                for command in available_extension.getElementsByTagName("command"):
                    names.append(command.getAttribute("name"))

    return set(names)


dom = parseString(download(args.cl_xml) if args.cl_xml.startswith("http") else open(args.cl_xml).read())
supported_exts = [] if args.cl_ext is None else list(filter(lambda l: not l.startswith("#"), [l.strip() for l in open(args.cl_ext)]))
supported_func_names = get_supported_func_names(dom, args.cl_std, supported_exts)
xml_commands = dom.getElementsByTagName("commands")
assert len(xml_commands) == 1
xml_commands = xml_commands[0].getElementsByTagName("command")

class Type:
    def __init__(self, prefix, suffix=""):
        self.prefix = prefix.strip()
        self.suffix = suffix.strip()

    def with_name(self, name):
        return f"{self.prefix} {name}{self.suffix}"

    def __str__(self):
        return self.prefix + self.suffix

    def __repr__(self):
        if self.suffix:
            s = self.with_name("/*unnamed*/")
        else:
            s = self.prefix
        return f"'{s}'"


class NameAndType:
    def __init__(self, name: str, type: Type):
        self._name = name
        self._type = type

    @property
    def name(self):
        return self._name

    @property
    def type(self):
        return self._type


class API:
    def __init__(self, command):
        self.command = command

    @staticmethod
    def from_xml_command(xml_command):
        def visit_proto_or_param(proto_or_param):
            name = None
            ret_type = ["", ""]  # prefix, suffix
            tidx = 0
            for n in proto_or_param.childNodes:
                if n.nodeType == n.TEXT_NODE:
                    t = n.wholeText.strip()
                    if t == "*":
                        ret_type[tidx] += t
                    else:
                        ret_type[tidx] += " " + t
                elif n.nodeType == n.ELEMENT_NODE:
                    if n.nodeName == "type":
                        ret_type[tidx] += " " + n.childNodes[0].wholeText.strip()
                    elif n.nodeName == "name":
                        name = n.childNodes[0].wholeText.strip()
                        assert tidx == 0, "proto or param should only have one name"
                        tidx += 1
                    else:
                        raise ValueError(n)
                else:
                    raise ValueError(n)
            return NameAndType(name, Type(*ret_type))

        # print(command.attributes["suffix"].value)
        # print(xml_command.toprettyxml())
        proto, = xml_command.getElementsByTagName("proto")
        ret = [visit_proto_or_param(proto)]
        for param in xml_command.getElementsByTagName("param"):
            ret.append(visit_proto_or_param(param))
        return API(ret)

    @property
    def name(self):
        return self.command[0].name

    @property
    def rettype(self) -> Type:
        return self.command[0].type

    @property
    def params(self) -> List[NameAndType]:
        return self.command[1:]

    def get_pfn_typedef_name(self):
        return "PFN" + self.name.upper() + "FUNC"

    def as_pfn_typedef(self):  # pointer to function
        typedef_name = self.get_pfn_typedef_name()
        prefix = str(self.rettype) + " (CL_API_CALL CL_API_ENTRYP"
        suffix = ")(" + ", ".join([param.type.with_name(param.name) for param in self.params]) + ")"
        return "typedef " + Type(prefix, suffix).with_name(typedef_name) + ";"

    def get_dummy_impl_name(self):
        return self.name + "DummyImpl"

    def is_rettype_err(self):
        is_nullary = len(self.params) == 0
        last_is_err_code = not is_nullary and str(self.params[-1].type) != "cl_int*"
        return str(self.rettype) == "cl_int" and (is_nullary or last_is_err_code)

    def is_last_param_err(self):
        is_nullary = len(self.params) == 0
        return not is_nullary and str(self.params[-1].type) == "cl_int*"

commands = [c for c in [API.from_xml_command(x) for x in xml_commands] if c.name in supported_func_names]


def gen_pfn_typedefs():
    s = io.StringIO()
    for command in commands:
        s.write(command.as_pfn_typedef())
        s.write("\n")
    return s.getvalue()


def gen_stub_decl():
    s = io.StringIO()
    s.write(indent(0) + "union CL3WAPIs {\n")
    s.write(indent(1) + f"CL3WclAPI ptr[{len(commands)}];\n")
    s.write(indent(1) + "struct {\n")
    for command in commands:
        s.write(indent(2) + "{:<55} {};\n".format(command.get_pfn_typedef_name(), command.name))
    s.write(indent(1) + "} cl;\n")
    s.write("};")
    return s.getvalue()


def gen_stub_impls():
    s = io.StringIO()
    for i, command in enumerate(commands):
        if i != 0:
            s.write("\n")
        formal = ", ".join([param.type.with_name(param.name) for param in command.params])
        actual = ", ".join([param.name for param in command.params])
        s.write(f"""{command.rettype} {command.name}({formal}) {{
{indent(1)}return cl3w_apis.cl.{command.name}({actual});
}}
""")
    return s.getvalue()


def gen_stub_dummies():
    s = io.StringIO()
    s.write('char cl3w_msg_prefix[] = "[cl3w] OpenCL API";\n')
    s.write('char cl3w_msg_suffix[] = "is not loaded/supported";\n')
    s.write("\n")
    for i, command in enumerate(commands):
        if i != 0:
            s.write("\n")

        formal = ", ".join([param.type.with_name(param.name) for param in command.params])

        if command.is_rettype_err():
            appendix = "return CL_INVALID_HOST_PTR;"
        elif command.is_last_param_err():
            appendix = f"*{command.params[-1].name} = CL_INVALID_HOST_PTR;\n{indent(1)}return NULL;"
        else:
            appendix = f"/* no error handling here*/\n{indent(1)}return NULL;"

        s.write(f"""{command.rettype} {command.get_dummy_impl_name()}({formal}) {{
{indent(1)}fprintf(stderr, "%s %s %s\\n", cl3w_msg_prefix, cl3w_api_names[{i}], cl3w_msg_suffix);
{indent(1)}/* We reuse CL_INVALID_HOST_PTR as an indicator of unloaded function, aka, function pointer is invalid. */
{indent(1)}{appendix}
}}
""")
    return s.getvalue()


def gen_array_cl3w_api_names():
    s = io.StringIO()
    s.write("static const char* cl3w_api_names[] = {\n")
    for command in commands:
        s.write(f'{indent(1)}"{command.name}",\n')
    s.write("};\n")
    return s.getvalue()


def gen_defines():
    s = io.StringIO()
    for command in commands:
        s.write("#define {0:<50} cl3w_apis.cl.{0}\n".format(command.name))
    return s.getvalue()


def gen_func_unload_apis():
    s = io.StringIO()
    s.write(indent(0) + "static void unload_apis(void) {\n")
    for i, command in enumerate(commands):
        s.write(indent(1) + f"cl3w_apis.ptr[{i}] = (CL3WclAPI){command.get_dummy_impl_name()};\n")
    s.write(indent(0) + "}\n")
    return s.getvalue()


def gen_func_get_probe_api_name():
    name = "clCreateContext"
    for i, command in enumerate(commands):
        if command.name == name:
            break
    s = io.StringIO()
    s.write(indent(0) + "static const char* get_probe_api_name(void) {\n")
    s.write(indent(1) + f"/* {name} */\n")
    s.write(indent(1) + f"return cl3w_api_names[{i}];\n")
    s.write(indent(0) + "}\n")
    return s.getvalue()


def touch_dir(relative_path):
    path = os.path.join(args.root, relative_path)
    os.makedirs(path, exist_ok=True)
    return path


class Template:
    def __init__(self, path):
        self.tpl = open(path).read()

    def format(self, key, value):
        full_key = f"/* generated {key} */\n"
        assert full_key in self.tpl, repr(full_key) + " not found"
        self.tpl = self.tpl.replace(full_key, value)

    def write(self, path):
        with open(path, "w") as f:
            if not args.no_header:
                if args.no_license:
                    f.write(NO_LICENSE_HEADER)
                else:
                    f.write(HEADER)
            f.write(self.tpl)


# print(gen_pfn_typedefs())
# print(gen_stub_decl())
# print(gen_defines())
# print(gen_stub_impls())
# print(gen_stub_dummies())
# print(gen_array_cl3w_api_names())
# print(gen_func_unload_apis())


include_dir = touch_dir("include")
cl3w_h = Template(os.path.join(os.path.dirname(__file__), "template", "cl3w.h"))
cl3w_h.format("typedefs", gen_pfn_typedefs())
cl3w_h.format("CL3WAPIs", gen_stub_decl())
cl3w_h.format("defines", gen_defines())
cl3w_h.write(os.path.join(include_dir, "cl3w.h"))
del cl3w_h


src_dir = touch_dir("src")
cl3w_c = Template(os.path.join(os.path.dirname(__file__), "template", "cl3w.c"))
cl3w_c.format("stub dummies", gen_stub_dummies())
cl3w_c.format("stub impls", gen_stub_impls())
cl3w_c.format("cl3w_api_names", gen_array_cl3w_api_names())
cl3w_c.format("unload_apis", gen_func_unload_apis())
cl3w_c.format("get_probe_api_name", gen_func_get_probe_api_name())
cl3w_c.write(os.path.join(src_dir, "cl3w.c"))
del cl3w_c
