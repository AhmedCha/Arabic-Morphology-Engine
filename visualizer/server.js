#!/usr/bin/env node
/**
 * AVL Tree Visualizer Server
 * Bridges the C++ terminal app <-> Browser via WebSocket
 *
 * Events file: /tmp/avl_events.jsonl  (C++ writes, we tail)
 * Command file: /tmp/avl_cmd.txt      (browser writes, C++ polls)
 */

const http = require("http");
const fs = require("fs");
const path = require("path");
const { WebSocketServer } = require("ws");

const EVENTS_FILE = "/tmp/avl_events.jsonl";
const CMD_FILE = "/tmp/avl_cmd.txt";
const PUBLIC_DIR = path.join(__dirname, "public");
const HTTP_PORT = 3000;
const WS_PORT = 3001;

// ─── MIME types ────────────────────────────────────────────────────────────
const MIME = {
  ".html": "text/html; charset=utf-8",
  ".css": "text/css",
  ".js": "application/javascript",
  ".json": "application/json",
  ".ico": "image/x-icon",
  ".svg": "image/svg+xml",
};

// ─── HTTP Server (serves the frontend) ──────────────────────────────────────
const httpServer = http.createServer((req, res) => {
  let reqPath = req.url === "/" ? "/index.html" : req.url;
  const filePath = path.join(PUBLIC_DIR, reqPath);

  fs.readFile(filePath, (err, data) => {
    if (err) {
      res.writeHead(404, { "Content-Type": "text/plain" });
      return res.end("404 Not Found");
    }
    const ext = path.extname(filePath);
    const mime = MIME[ext] || "application/octet-stream";
    res.writeHead(200, { "Content-Type": mime });
    res.end(data);
  });
});

httpServer.listen(HTTP_PORT, () => {
  console.log(`\x1b[36m🌐 Frontend: http://localhost:${HTTP_PORT}\x1b[0m`);
});

// ─── WebSocket Server ────────────────────────────────────────────────────────
const wss = new WebSocketServer({ port: WS_PORT });
const clients = new Set();

function broadcast(obj) {
  const msg = JSON.stringify(obj);
  for (const client of clients) {
    if (client.readyState === 1) client.send(msg);
  }
}

wss.on("connection", (ws) => {
  clients.add(ws);
  console.log(`\x1b[32m✔ Browser connected (${clients.size} total)\x1b[0m`);

  // Send latest snapshot to newly connected client
  sendLatestSnapshot(ws);

  ws.on("message", (raw) => {
    try {
      const msg = JSON.parse(raw.toString());
      handleBrowserMessage(msg);
    } catch (e) {
      console.error("Bad WS message:", e.message);
    }
  });

  ws.on("close", () => {
    clients.delete(ws);
    console.log(
      `\x1b[33m• Browser disconnected (${clients.size} remaining)\x1b[0m`,
    );
  });
});

console.log(`\x1b[36m🔌 WebSocket: ws://localhost:${WS_PORT}\x1b[0m`);

// ─── Handle commands from browser ───────────────────────────────────────────
function handleBrowserMessage(msg) {
  if (msg.type === "insert" || msg.type === "delete" || msg.type === "search") {
    // Write command to file so C++ can poll it
    const cmd = `${msg.type}:${msg.key}\n`;
    fs.appendFileSync(CMD_FILE, cmd);
    console.log(`\x1b[35m→ Browser cmd: ${msg.type} "${msg.key}"\x1b[0m`);

    // Also simulate locally for immediate UI feedback if C++ isn't running
    // (The C++ app will overwrite with authoritative state when it runs)
    processInternalTree(msg.type, msg.key);
  } else if (msg.type === "family") {
    const cmd = `family:${msg.key}\n`;
    fs.appendFileSync(CMD_FILE, cmd);
    console.log(`\x1b[35m→ Browser cmd: family "${msg.key}"\x1b[0m`);
  } else if (msg.type === "corpus") {
    // Save corpus chunk to a temp file and send its path to C++
    const tempPath = path.join("/tmp", `avl_corpus_${Date.now()}.txt`);
    fs.writeFileSync(tempPath, msg.text, "utf8");
    const cmd = `corpus:${tempPath}\n`;
    fs.appendFileSync(CMD_FILE, cmd);
    console.log(`\x1b[35m→ Browser cmd: corpus (${tempPath})\x1b[0m`);
  } else if (msg.type === "corpus-file") {
    const buffer = Buffer.from(msg.data, "base64");
    const ext = msg.name.toLowerCase().endsWith(".pdf") ? ".pdf" : ".txt";
    const tempPath = path.join("/tmp", `avl_corpus_${Date.now()}${ext}`);
    fs.writeFileSync(tempPath, buffer);
    const cmd = `corpus:${tempPath}\n`;
    fs.appendFileSync(CMD_FILE, cmd);
    console.log(`\x1b[35m→ Browser cmd: corpus-file (${tempPath})\x1b[0m`);
  }
}

// ─── In-memory tree mirror (for standalone browser mode) ────────────────────
let internalTree = { root: null };

function processInternalTree(op, key) {
  if (op === "insert") {
    const steps = avlInsertWithSteps(internalTree, key);
    for (const step of steps) {
      broadcast(step);
    }
  } else if (op === "delete") {
    const steps = avlDeleteWithSteps(internalTree, key);
    for (const step of steps) {
      broadcast(step);
    }
  }
}

// ─── Watch the events file from C++ ─────────────────────────────────────────
let filePosition = 0;

// Ensure the events file exists
if (!fs.existsSync(EVENTS_FILE)) {
  fs.writeFileSync(EVENTS_FILE, "");
}

// Poll the events file every 100ms for new lines
setInterval(() => {
  try {
    const stat = fs.statSync(EVENTS_FILE);
    if (stat.size > filePosition) {
      const fd = fs.openSync(EVENTS_FILE, "r");
      const toRead = stat.size - filePosition;
      const buf = Buffer.alloc(toRead);
      fs.readSync(fd, buf, 0, toRead, filePosition);
      fs.closeSync(fd);
      filePosition = stat.size;

      const lines = buf.toString("utf8").split("\n");
      for (const line of lines) {
        const trimmed = line.trim();
        if (!trimmed) continue;
        try {
          const event = JSON.parse(trimmed);
          // Update our internal tree mirror if it's a snapshot
          if (event.type === "snapshot" && event.tree !== undefined) {
            internalTree.root = event.tree;
          }
          broadcast(event);
        } catch (e) {
          // ignore malformed lines
        }
      }
    }
  } catch (e) {
    // file might not exist yet
  }
}, 100);

// ─── Send snapshot to newly connected client ─────────────────────────────────
function sendLatestSnapshot(ws) {
  // Send current in-memory state
  if (internalTree.root !== null) {
    ws.send(
      JSON.stringify({
        type: "snapshot",
        tree: internalTree.root,
        op: null,
      }),
    );
    return;
  }

  // Or read the last snapshot line from events file
  try {
    const content = fs.readFileSync(EVENTS_FILE, "utf8");
    const lines = content.split("\n").filter((l) => l.trim());
    for (let i = lines.length - 1; i >= 0; i--) {
      try {
        const ev = JSON.parse(lines[i]);
        if (ev.type === "snapshot") {
          internalTree.root = ev.tree;
          ws.send(JSON.stringify(ev));
          break;
        }
      } catch (e) {}
    }
  } catch (e) {}
}

// ─── JS AVL tree (mirrors C++ logic for standalone mode) ────────────────────
function nodeHeight(n) {
  return n ? n.height : 0;
}
function balanceFactor(n) {
  return n ? nodeHeight(n.left) - nodeHeight(n.right) : 0;
}
function updateHeight(n) {
  if (n) n.height = 1 + Math.max(nodeHeight(n.left), nodeHeight(n.right));
}

function rightRotate(y) {
  const x = y.left;
  const T2 = x.right;
  x.right = y;
  y.left = T2;
  updateHeight(y);
  updateHeight(x);
  return { newRoot: x, pivot: x.key };
}

function leftRotate(x) {
  const y = x.right;
  const T2 = y.left;
  y.left = x;
  x.right = T2;
  updateHeight(x);
  updateHeight(y);
  return { newRoot: y, pivot: y.key };
}

function createNode(key) {
  return { key, left: null, right: null, height: 1 };
}

function avlInsertWithSteps(tree, key) {
  const steps = [];

  function insert(node, key) {
    if (!node) {
      const n = createNode(key);
      steps.push({ type: "node_inserted", key, state: "new" });
      return n;
    }
    if (key < node.key) {
      node.left = insert(node.left, key);
    } else if (key > node.key) {
      node.right = insert(node.right, key);
    } else {
      return node; // duplicate
    }

    updateHeight(node);
    const bf = balanceFactor(node);

    if (bf > 1 && key < node.left.key) {
      steps.push({
        type: "rotation",
        rotation: "LL",
        pivot: node.left.key,
        tree: JSON.parse(JSON.stringify(tree.root)),
      });
      const { newRoot } = rightRotate(node);
      return newRoot;
    }
    if (bf < -1 && key > node.right.key) {
      steps.push({
        type: "rotation",
        rotation: "RR",
        pivot: node.right.key,
        tree: JSON.parse(JSON.stringify(tree.root)),
      });
      const { newRoot } = leftRotate(node);
      return newRoot;
    }
    if (bf > 1 && key > node.left.key) {
      steps.push({
        type: "rotation",
        rotation: "LR",
        pivot: node.left.key,
        tree: JSON.parse(JSON.stringify(tree.root)),
      });
      const lr = leftRotate(node.left);
      node.left = lr.newRoot;
      const { newRoot } = rightRotate(node);
      return newRoot;
    }
    if (bf < -1 && key < node.right.key) {
      steps.push({
        type: "rotation",
        rotation: "RL",
        pivot: node.right.key,
        tree: JSON.parse(JSON.stringify(tree.root)),
      });
      const rr = rightRotate(node.right);
      node.right = rr.newRoot;
      const { newRoot } = leftRotate(node);
      return newRoot;
    }
    return node;
  }

  tree.root = insert(tree.root, key);
  steps.push({
    type: "snapshot",
    tree: JSON.parse(JSON.stringify(tree.root)),
    op: "insert",
    key,
  });
  return steps;
}

function avlDeleteWithSteps(tree, key) {
  const steps = [];
  steps.push({ type: "node_deleting", key });

  function minValueNode(node) {
    let cur = node;
    while (cur.left) cur = cur.left;
    return cur;
  }

  function deleteNode(node, key) {
    if (!node) return null;

    if (key < node.key) {
      node.left = deleteNode(node.left, key);
    } else if (key > node.key) {
      node.right = deleteNode(node.right, key);
    } else {
      if (!node.left || !node.right) {
        const temp = node.left || node.right;
        if (!temp) return null;
        else return temp;
      } else {
        const temp = minValueNode(node.right);
        node.key = temp.key;
        node.right = deleteNode(node.right, temp.key);
      }
    }

    if (!node) return node;
    updateHeight(node);
    const bf = balanceFactor(node);

    if (bf > 1 && balanceFactor(node.left) >= 0) {
      steps.push({
        type: "rotation",
        rotation: "LL",
        pivot: node.left.key,
        tree: JSON.parse(JSON.stringify(tree.root)),
      });
      return rightRotate(node).newRoot;
    }
    if (bf > 1 && balanceFactor(node.left) < 0) {
      steps.push({
        type: "rotation",
        rotation: "LR",
        pivot: node.left.key,
        tree: JSON.parse(JSON.stringify(tree.root)),
      });
      node.left = leftRotate(node.left).newRoot;
      return rightRotate(node).newRoot;
    }
    if (bf < -1 && balanceFactor(node.right) <= 0) {
      steps.push({
        type: "rotation",
        rotation: "RR",
        pivot: node.right.key,
        tree: JSON.parse(JSON.stringify(tree.root)),
      });
      return leftRotate(node).newRoot;
    }
    if (bf < -1 && balanceFactor(node.right) > 0) {
      steps.push({
        type: "rotation",
        rotation: "RL",
        pivot: node.right.key,
        tree: JSON.parse(JSON.stringify(tree.root)),
      });
      node.right = rightRotate(node.right).newRoot;
      return leftRotate(node).newRoot;
    }
    return node;
  }

  tree.root = deleteNode(tree.root, key);
  steps.push({
    type: "snapshot",
    tree: JSON.parse(JSON.stringify(tree.root)),
    op: "delete",
    key,
  });
  return steps;
}

console.log(`\n\x1b[1m\x1b[36m╔══════════════════════════════════╗\x1b[0m`);
console.log(`\x1b[1m\x1b[36m║  AVL Tree Visualizer Server      ║\x1b[0m`);
console.log(`\x1b[1m\x1b[36m╚══════════════════════════════════╝\x1b[0m\n`);
console.log(`Watching events: ${EVENTS_FILE}`);
console.log(`Commands file:   ${CMD_FILE}\n`);
