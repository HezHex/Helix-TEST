import os
Import("env")


def before_upload(source, target, env):
    print("#############")
    print("before_upload")
    print("#############")
    os.system('.\\Helix_Helper_Bridge.exe Helix_Dev_Suite $DISCONNECT ')
    

    


def after_upload(source, target, env):
    print("#############")
    print("after_upload")
    print("#############")
    # do some actions
    os.system(
        '.\\Helix_Helper_Bridge.exe Helix_Dev_Suite $CONNECT ' 
        + env.get("UPLOAD_PORT") + " "                                          ### UPLOAD Port (Com3)
        + str(env.GetProjectOption("monitor_speed"))                            ### Monitor Speed
        )
    
env.AddPreAction("upload", before_upload)
env.AddPostAction("upload", after_upload)
