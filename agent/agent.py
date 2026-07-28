import os
from openai import OpenAI, OpenAIError
from mcp_tools import most_data_located
import json

api_key = os.getenv("OPENROUTER_API_KEY")
if not api_key:
    raise RuntimeError("OPENROUTER_API_KEY is not set.")

client = OpenAI(
    base_url="https://openrouter.ai/api/v1",
    api_key=api_key,
)

model="inclusionai/ling-3.0-flash:free"

SYSTEM_PROMPT = """
You are a grid workload-management scheduling agent.

Your task is to send each incoming job to the grid site where most of the
job's required input data is located.

The only valid grid sites are Site0 through Site29.

You have access to exactly one tool:

most_data_located

For every incoming job, follow this procedure:

1. Call `most_data_located` exactly once for the job.
2. Use the site returned by the tool as the destination site.
3. Do not call any other tool.
4. Do not call `most_data_located` more than once.
5. Do not independently choose, rank, or replace the returned site.
6. Never invent a site or tool result.
7. The selected site must be one of Site0 through Site29.
8. If the tool returns an invalid site or an error, set `selected_site` to null.

Return only valid JSON in this exact format:

{
"site_decision": "Site0",
"reason": "Most of the job's required data is located at Site0."
}

Output rules:

* `selected_site` must exactly match the valid site returned by
  `most_data_located`.
* Do not include alternative sites.
* Do not include Markdown or code fences.
* Do not include any text outside the JSON object.
  """

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
