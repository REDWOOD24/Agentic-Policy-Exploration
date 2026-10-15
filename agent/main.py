from server import get_server
from agent import pick_policies
server=get_server()

while True:
    message=server.receive_json()
    if message is None: continue
    print("Communication from Grid: ",message)
    request_type = message.get("request_type")
    if(request_type=="end_simulation"): break
    assert(request_type == "pick_data_management_policy")
    reply ={"decision": pick_policies()}
    print("Communication to Grid: ",reply)
    server.send_json(reply)



