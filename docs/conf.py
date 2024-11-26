from clang.cindex import Config
Config.set_library_file("libclang.so")

project = "uWAN Documentation"
copyright = "2024, Alexey Ryabov"
author = "Alexey Ryabov"

extensions = [
    "sphinx_c_autodoc",
    "myst_parser",
]

templates_path = ["_templates"]
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store"]

html_theme = "alabaster"
html_static_path = ["_static"]

todo_include_todos = True

c_autodoc_roots = ["../include/uwan", "../src"]
c_autodoc_compilation_database = "../build/compile_commands.json"
