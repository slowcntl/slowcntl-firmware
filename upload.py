from shutil import copyfile

Import('env')

def upload(source, target, env):
    print('Uploading firmware')
    print(source[0])
    print(env['UPLOAD_PORT'])
    copyfile(str(source[0]), env['UPLOAD_PORT'])

env.Replace(
    UPLOADCMD=upload
)
