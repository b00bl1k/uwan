from clang.cindex import Config

project = "uWAN Documentation"
copyright = "2024, Alexey Ryabov"
author = "Alexey Ryabov"

extensions = [
    "sphinx_c_autodoc",
]

templates_path = ["_templates"]
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store"]

html_theme = "alabaster"
html_theme_options = {
    "github_user": "b00bl1k",
    "github_repo": "uwan",
}
html_static_path = ["_static"]

todo_include_todos = True

c_autodoc_roots = ["../include/uwan", "../src"]
c_autodoc_compilation_database = "../build/compile_commands.json"
