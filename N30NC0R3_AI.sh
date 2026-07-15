import json
import math
import os
import re
import urllib.parse
import urllib.request
import datetime
import html
import base64
import sys

# NEON_HISTORY_START
NEON_HISTORY_B64 = 'eyJjaGF0cyI6IHsiMSI6IHsibmFtZSI6ICJOZXcgU2Vzc2lvbiIsICJ0aW1lIjogIjIwMjYtMDYtMjVUMjA6Mzk6MDEuODg0MTE5In19LCAibWVzc2FnZXMiOiB7IjEiOiBbeyJyb2xlIjogInVzZXIiLCAidGV4dCI6ICJiYW5hbmEifSwgeyJyb2xlIjogImJvdCIsICJ0ZXh0IjogIlx1ZDgzZFx1ZGNhMSAqKkVuY3ljbG9wZWRpYToqKlxuQSBiYW5hbmEgaXMgYW4gZWxvbmdhdGVkLCBlZGlibGUgZnJ1aXQgdGhhdCBpcyBib3RhbmljYWxseSBhIGJlcnJ5IHByb2R1Y2VkIGJ5IHNldmVyYWwga2luZHMgb2YgbGFyZ2UgdHJlZWxpa2UgaGVyYmFjZW91cyBmbG93ZXJpbmcgcGxhbnRzIGluIHRoZSBnZW51cyBNdXNhLiBJbiBzb21lIGNvdW50cmllcywgY29va2luZyBiYW5hbmFzIGFyZSBjYWxsZWQgcGxhbnRhaW5zLCBkaXN0aW5ndWlzaGluZyB0aGVtIGZyb20gZGVzc2VydCBiYW5hbmFzLiBUaGUgZnJ1aXQgaXMgdmFyaWFibGUgaW4gc2l6ZSwgY29sb3IgYW5kIGZpcm1uZXNzLCBidXQgaXMgdXN1YWxseSBlbG9uZ2F0ZWQgYW5kIGN1cnZlZCwgd2l0aCBzb2Z0IGZsZXNoIHJpY2ggaW4gc3RhcmNoIGNvdmVyZWQgd2l0aCBhIHBlZWwsIHdoaWNoIG1heSBoYXZlIGEgdmFyaWV0eSBvZiBjb2xvcnMgd2hlbiByaXBlLiBJdCBncm93cyB1cHdhcmQgaW4gY2x1c3RlcnMgbmVhciB0aGUgdG9wIG9mIHRoZSBwbGFudC4gQWxtb3N0IGFsbCBtb2Rlcm4gZWRpYmxlIHNlZWRsZXNzIChwYXJ0aGVub2NhcnApIGN1bHRpdmF0ZWQgYmFuYW5hcyBjb21lIGZyb20gdHdvIHdpbGQgc3BlY2llcyBcdTIwMTMgTXVzYSBhY3VtaW5hdGEgYW5kIE11c2EgYmFsYmlzaWFuYSwgb3IgdGhlaXIgaHlicmlkcy4ifV19LCAibmV4dF9pZCI6IDJ9'
# NEON_HISTORY_END
# NEON_SETTINGS_START
NEON_SETTINGS_B64 = 'eyJ0aGVtZSI6ICJOZW9uIFBpbmsiLCAiaW50ZWxsaWdlbmNlIjogIkJhbGFuY2VkIiwgInJlc3BvbnNlX3N0eWxlIjogIkJhbGFuY2VkIiwgInNwZWVkIjogIk5vcm1hbCIsICJyZXN1bHRfY291bnQiOiA1LCAiZm9udF9zaXplIjogMTIsICJ3ZWJfc2VhcmNoIjogdHJ1ZSwgIndpa2lwZWRpYSI6IHRydWUsICJsaXZlX3NlYXJjaCI6IHRydWUsICJnaXRodWJfc2VhcmNoIjogZmFsc2UsICJyZWRkaXRfc2VhcmNoIjogZmFsc2UsICJuZXdzX3NlYXJjaCI6IGZhbHNlLCAiYWRfZmlsdGVyIjogdHJ1ZSwgInNhdmVfbWVtb3J5IjogdHJ1ZSwgInNhdmVfbG9jYWxseSI6IGZhbHNlLCAic2F2ZV9wYXRoIjogIiJ9'
# NEON_SETTINGS_END

def _decode_store(b64_str):
    try: return json.loads(base64.b64decode(b64_str).decode('utf-8'))
    except: return {}

NEON_HISTORY = _decode_store(NEON_HISTORY_B64)
NEON_SETTINGS = _decode_store(NEON_SETTINGS_B64)

class ThinkingBrain:
    def __init__(self, settings=None):
        self.user_agent = 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36'
        self.blacklist = ["ebay", "amazon", "aliexpress"]
        self.ad_triggers = ["price", "buy now"]
        self.settings = settings or {}

    def _fetch(self, url, timeout=12):
        try:
            req = urllib.request.Request(url, headers={'User-Agent': self.user_agent})
            with urllib.request.urlopen(req, timeout=timeout) as response:
                return response.read().decode('utf-8', errors='ignore')
        except: return ""

    def _clean(self, text):
        text = html.unescape(re.sub(r'<.*?>', '', text))
        return re.sub(r'\s+', ' ', text).strip()

    def respond(self, query, history):
        q_low = query.lower().strip()
        if any(k in q_low for k in ["who are you", "your name"]):
            return "I am N30N C0RE AI"
        if any(q_low.startswith(g) for g in ["hi", "hello", "hey"]):
            return "N30N C0RE AI active"
        if q_low in ["time", "date"]:
            return f"It is {datetime.datetime.now().strftime('%I:%M %p %A %B %d %Y')}."

        results = []
        encoded = urllib.parse.quote(query)
        if self.settings.get("wikipedia", True):
            try:
                wiki = json.loads(self._fetch(f"https://en.wikipedia.org/api/rest_v1/page/summary/{urllib.parse.quote(query.replace(' ', '_'))}"))
                if wiki.get("extract"): results.append(("Encyclopedia", wiki["extract"]))
            except: pass
        if self.settings.get("web_search", True):
            try:
                html_data = self._fetch(f"https://html.duckduckgo.com/html/?q={encoded}")
                blocks = re.findall(r'<div class="result__body">(.*?)</div>', html_data, re.DOTALL)
                web_intel = []
                for block in blocks[:5]:
                    snippet_m = re.search(r'<a class="result__snippet".*?>(.*?)</a>', block, re.DOTALL)
                    if snippet_m:
                        snippet = self._clean(snippet_m.group(1))
                        if len(snippet) > 30: web_intel.append(snippet)
                if web_intel: results.append(("Web", web_intel))
            except: pass
        if not results: return "no data found try again"
        final = ""
        for tag, content in results:
            if isinstance(content, list):
                final += f"\n{tag}:\n" + "\n".join(f"• {s}" for s in content[:3])
            else: final += f"\n{tag}: {content}"
        return final.strip()

class MemoryDatabase:
    _storage = None
    def __init__(self, settings=None):
        self.settings = settings or {}
        if MemoryDatabase._storage is None:
            MemoryDatabase._storage = {"chats": {int(k): v for k, v in NEON_HISTORY.get("chats", {}).items()}, "messages": {int(k): v for k, v in NEON_HISTORY.get("messages", {}).items()}, "next_id": NEON_HISTORY.get("next_id", 1)}

    def get_or_create_chat(self): 
        if self._storage["chats"]: return sorted(self._storage["chats"].keys())[0]
        return self.create_chat("cli")

    def create_chat(self, name):
        cid = self._storage["next_id"]; self._storage["next_id"] += 1
        self._storage["chats"][cid] = {"name": name, "time": datetime.datetime.now().isoformat()}
        self._storage["messages"][cid] = []; self._save(); return cid

    def add_msg(self, cid, role, text):
        if cid in self._storage["messages"]:
            self._storage["messages"][cid].append({"role": role, "text": text})
            self._save()

    def get_history(self, cid): return self._storage["messages"].get(cid, [])

    def _save(self):
        try:
            path = os.path.abspath(__file__)
            with open(path, "r") as f: source = f.read()
            b64 = base64.b64encode(json.dumps(self._storage).encode('utf-8')).decode('utf-8')
            source = re.sub(r"# NEON_HISTORY_START\nNEON_HISTORY_B64 = .*?\n# NEON_HISTORY_END", f"# NEON_HISTORY_START\nNEON_HISTORY_B64 = '{b64}'\n# NEON_HISTORY_END", source, flags=re.DOTALL)
            with open(path, "w") as f: f.write(source)
        except: pass

def main():
    settings = NEON_SETTINGS
    memory = MemoryDatabase(settings)
    chat_id = memory.get_or_create_chat()
    brain = ThinkingBrain(settings)
    print("neon cli ready bro type exit to quit")
    while True:
        try:
            query = input("> ").strip()
            if query.lower() in ["exit", "quit"]: break
            if not query: continue
            print("you:", query)
            memory.add_msg(chat_id, "user", query)
            resp = brain.respond(query, memory.get_history(chat_id))
            print("neon:", resp)
            memory.add_msg(chat_id, "bot", resp)
        except: 
            break

if __name__ == "__main__":
    main()