from server import get_server

server=get_server()

def calculate_storage_imbalance():
    request = {"request_type":"tool", "tool_type":"calculate_storage_imbalance"}
    print("Communication to Grid: ",request)
    server.send_json(request)
    response = server.receive_json()
    print("Communication from Grid: ",response)
    return response.get("tool_result")
