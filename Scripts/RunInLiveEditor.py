"""Execute a project Python script in an already-running local Unreal Editor session."""

import os
import sys
import time


ENGINE_PYTHON_PATH = r"E:\Program Files (x86)\UE_5.8\Engine\Plugins\Experimental\PythonScriptPlugin\Content\Python"
if ENGINE_PYTHON_PATH not in sys.path:
    sys.path.insert(0, ENGINE_PYTHON_PATH)

import remote_execution


if len(sys.argv) != 2:
    raise RuntimeError("Usage: RunInLiveEditor.py <project-python-script>")

script_path = os.path.abspath(sys.argv[1])
if not os.path.isfile(script_path):
    raise RuntimeError("Script does not exist: {}".format(script_path))

remote = remote_execution.RemoteExecution()
remote.start()
try:
    time.sleep(2.0)
    nodes = remote.remote_nodes
    if not nodes:
        raise RuntimeError("No running Unreal Editor with Python Remote Execution was found.")

    project_node = next((node for node in nodes if node.get("project_name") == "GraduationProject"), nodes[0])
    remote.open_command_connection(project_node["node_id"])
    command = "exec(compile(open({0!r}, encoding='utf-8').read(), {0!r}, 'exec'))".format(script_path)
    result = remote.run_command(command, exec_mode=remote_execution.MODE_EXEC_STATEMENT, raise_on_failure=True)
    print(result)
finally:
    remote.stop()
