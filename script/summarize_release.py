import os
import sys
import subprocess
from litellm import completion

# SET OPENAI_API_KEY / corrosponding provider api key variables
model= os.environ.get("LLM_MODEL", "openai/gpt-4o-mini")
end = sys.argv[1] if len(sys.argv) > 1 else "HEAD~20" 
start = sys.argv[2] if len(sys.argv) > 2 else "HEAD" 


def get_commit_messages(start_ref: str, end_ref: str) -> str:
    result = subprocess.run(
        ["git", "log", f"{start_ref}..{end_ref}", "--pretty=format:%s"],
        capture_output=True,
        text=True,
        check=True
    )
    return result.stdout.strip()

def gen_prompt(commit_log: str) -> str:
    return f"""
        summarize this commit log to provide high level release notes. Generate your response in markdown and split it into platform specific and independant sections. Also write in one paragraph a summary of all changes. Note that: 
        - (#XXXXX) is a link to a PR and should be retained 
        - there may be some technical changes / non-functional changes that make only sense to developers. Please move those to a dedicated section too 

        Answer by just returning a summary. No narration, comments or questions. Answer should be formatted in markdown

        Here is the commit log:
        {commit_log}
    """



log = get_commit_messages(start, end)
prompt = gen_prompt(log)
messages = [{ "content": prompt}]

# openai call
response = completion(model=model, messages=messages)
print(response.choices[0].message.content)