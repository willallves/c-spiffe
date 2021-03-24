import os
import time


def before_all(context):
    context.spiffe_id = context.config.userdata['spiffe_id']
    context.server_conf = context.config.userdata['server_conf']


def after_all(context):
    os.system("./clean.sh")
    time.sleep(1)