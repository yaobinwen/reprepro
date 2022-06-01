import io
import os.path as path
import ruamel.yaml as yaml
import shutil
import subprocess
import tempfile


M = """
int main(int argc, char * argv[]) {
    return 0;
}
"""


def verify_target(fname, deps, delete):
    try:
        dpath = tempfile.mkdtemp(prefix="tmp.")
        shutil.copy2(path.join("..", fname), path.join(dpath, fname))
        for dep in deps:
            shutil.copy2(path.join("..", dep), path.join(dpath, dep))

        fmain = path.join(dpath, "main.c")
        with io.open(fmain, mode="w", encoding="utf-8") as fh:
            fh.write(f"#include \"{fname}\"")
            fh.write("\n")
            fh.write(M)

        subprocess.run(
            ["gcc", fmain],
            cwd=dpath,
        )

    finally:
        if delete:
            shutil.rmtree(dpath, ignore_errors=True)


def main():
    with io.open("./targets.yml", mode="r", encoding="utf-8") as fh:
        config = yaml.YAML().load(fh.read())

    targets = config["targets"]
    for target in targets:
        verify_target(fname=target["name"], deps=target["deps"], delete=True)


if __name__ == "__main__":
    main()
