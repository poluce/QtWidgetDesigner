import asyncio
import json
import sys

try:
    import websockets
except ModuleNotFoundError:
    print("manual_client.py requires the 'websockets' package.")
    print("For dependency-free integration, use QtAgentMcpServer.exe via MCP instead.")
    raise SystemExit(2)


async def main() -> None:
    if len(sys.argv) < 2:
        print("Usage: python tools/manual_client.py '{\"command\":\"ping\"}'")
        raise SystemExit(1)

    payload = json.loads(sys.argv[1])
    payload.setdefault("id", "manual-client")

    async with websockets.connect("ws://127.0.0.1:49555") as ws:
        await ws.send(json.dumps(payload, ensure_ascii=False))
        response = await ws.recv()
        print(response)


if __name__ == "__main__":
    asyncio.run(main())
