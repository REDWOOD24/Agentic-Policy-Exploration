import os
from openai import OpenAI # type: ignore
from mcp_tools import most_data_located
import json
from system_prompt import SYSTEM_PROMPT

api_key = os.getenv("BNL_API_KEY")
if not api_key:
    raise RuntimeError("BNL_API_KEY is not set.")

client = OpenAI(
    base_url="https://inference0-api.sdcc.bnl.gov/v1",
    api_key=api_key,
)

model="gpt-oss-120b"
functions={"most_data_located":most_data_located}
tools=[{"type":"function","function":{"name":"most_data_located","description":"Return the site holding most job data.","parameters":{"type":"object","properties":{},"additionalProperties":False}}}]
 
def schedule(job_id):
    messages=[{"role":"system","content":SYSTEM_PROMPT},{"role":"user","content": "Schedule job: " + str(job_id)}]
    r=client.chat.completions.create(model=model,messages=messages,tools=tools,tool_choice="auto")
    m=r.choices[0].message

    if m.tool_calls:
        messages.append(m)
        for x in m.tool_calls:
            result=functions[x.function.name]()
            messages.append({"role":"tool","tool_call_id":x.id,"content":result})
        r=client.chat.completions.create(model=model,messages=messages,tools=tools,tool_choice="none")

    return json.loads(r.choices[0].message.content)["site_decision"]
