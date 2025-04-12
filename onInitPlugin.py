# This Python file uses the following encoding: utf-8

# if __name__ == "__main__":
#     pass
import maya.cmds as cmds

print("exec onInitPlugin")

def uv_editor_opened():
    print("UV editor opened")

uv_editor_listener = cmds.scriptJob(event=("texWindowEditorShowup", uv_editor_opened))
print(f"UV editor listener script job started with ID {uv_editor_listener}")