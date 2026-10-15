import os
import json
from openai import OpenAI
from system_prompt import SYSTEM_PROMPT
from server import get_server

api_key = os.getenv("BNL_API_KEY")
if not api_key: raise RuntimeError("BNL_API_KEY is not set.")

client = OpenAI(base_url="https://inference0-api.sdcc.bnl.gov/v1",api_key=api_key,)
model = "gpt-oss-120b"
server = get_server()

def tool_request(tool_name):
    request = {"request_type": "tool", "tool_type": tool_name}
    print(request)
    server.send_json(request)
    response = server.receive_json()
    print(response)
    return response.get("tool_result")

tools = [{"type": "function", "function": {
    "name": "calculate_storage_imbalance",
    "description": "Returns storage imbalance from 0 (balanced) to 1 (imbalanced).",
    "parameters": {"type": "object", "properties": {}, "additionalProperties": False}}}]

def pick_policies():
    messages = [
        {"role": "system", "content": SYSTEM_PROMPT},
        {"role": "user", "content": f"Pick Data Management Policies"}]

    r = client.chat.completions.create(
        model=model,
        messages=messages,
        tools=tools,
        tool_choice="auto")

    m = r.choices[0].message

    if m.tool_calls:
        messages.append(m.model_dump(exclude_none=True))

        for call in m.tool_calls:
            name = call.function.name
            result = tool_request(name)
            messages.append({
                "role": "tool",
                "tool_call_id": call.id,
                "content": (
                    result if isinstance(result, str)
                    else json.dumps(result))})

        r = client.chat.completions.create(
            model=model,
            messages=messages,
            tools=tools,
            tool_choice="none")

    content = r.choices[0].message.content
    print("LLM content:", repr(content))
    result = json.loads(content)

    if "decision" not in result:
        raise ValueError(f"Missing decision in response: {content}")

    return result["decision"]

