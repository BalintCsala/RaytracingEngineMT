import os
import subprocess
import shutil

def walk_file_tree(dir):
    for file in os.listdir(dir):
        path = f"{dir}/{file}"
        if os.path.isdir(path):
            walk_file_tree(f"{path}/")
        elif file.split(".")[-1] in ("comp", "rahit", "rchit", "rgen", "rmiss"):
            os.makedirs(f"../shaders-spv{dir}", exist_ok=True)
            subprocess.run([ "glslc", path, "-o", f"../shaders-spv/{path}.spv", "--target-env=vulkan1.3", "-I ./" ])
            print(f"Converting {path}")


def main():
    shutil.rmtree("../shaders-spv")
    walk_file_tree("./")

if __name__ == "__main__":
    main()
