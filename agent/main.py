from server import get_server
from agent import schedule
server=get_server()

while True:
    message=server.receive_json()
    if message is None: continue
    print("Comminication from Grid: ",message)
    request_type = message.get("request_type")
    job_id = message.get("job_id")
    assert(request_type == "assign_job")
    reply ={"site_decision": schedule(job_id)}
    print("Comminication to Grid: ",reply)
    server.send_json(reply)



