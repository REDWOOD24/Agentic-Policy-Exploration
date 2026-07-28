from server import get_server

server=get_server()

def most_data_located():
    request = {"request_type":"tool", "tool_type":"most_data_located"}
    print("Comminication to Grid: ",request)
    server.send_json(request)
    response = server.receive_json()
    print("Comminication from Grid: ",response)
    return response.get("tool_result")
