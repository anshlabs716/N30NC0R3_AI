# Data Handling

What data stays local and what may leave the device.

With the local backends (llama.cpp or a local Ollama server), prompts never leave
your computer. When **Use cloud AI** is enabled (the default), prompts are sent
over the internet to the configured cloud provider — the free keyless
Pollinations service unless you choose another provider and paste a key. Web
search also sends the query to DuckDuckGo. Turn cloud AI off to keep everything
local.
