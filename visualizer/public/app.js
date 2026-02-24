/**
 * AVL Tree Visualizer — Frontend App
 * D3.js v7 + WebSocket real-time sync
 */

"use strict";

// ─── Config ──────────────────────────────────────────────────────────────────
const WS_URL = `ws://${location.hostname}:3001`;
const ANIM_DURATION = 600; // ms per tree redraw step
const STEP_DELAY = 700; // ms between animation steps in a sequence
const NODE_RADIUS = 26;
const LEVEL_HEIGHT = 90;

// ─── State ────────────────────────────────────────────────────────────────────
let ws = null;
let wsReady = false;
let treeRoot = null; // current tree state (JS object mirroring C++ tree)
let animQueue = []; // pending animation events
let animating = false; // lock flag
let totalRotations = 0;
let totalOps = 0;
let highlightKey = null; // key to highlight (search result)

// ─── Stats counters ──────────────────────────────────────────────────────────
const stats = {
  nodes: 0,
  height: 0,
  rotations: 0,
  ops: 0,
};

// ─── D3 Setup ─────────────────────────────────────────────────────────────────
const svgEl = document.getElementById("tree-svg");
const svgD3 = d3.select("#tree-svg");
const canvasEl = document.getElementById("tree-canvas");

// Layers in z-order: links below nodes
const linkLayer = svgD3.append("g").attr("class", "link-layer");
const nodeLayer = svgD3.append("g").attr("class", "node-layer");

// Tooltip
const tooltip = document.createElement("div");
tooltip.className = "node-tooltip";
document.body.appendChild(tooltip);

// ─── WebSocket ────────────────────────────────────────────────────────────────
function connectWS() {
  ws = new WebSocket(WS_URL);

  ws.onopen = () => {
    wsReady = true;
    setConnectionState(true);
    addLog("system", "⚡", "Connected to backend", "WebSocket ready");
  };

  ws.onclose = () => {
    wsReady = false;
    setConnectionState(false);
    addLog("system", "○", "Disconnected", "Retrying in 2s…");
    setTimeout(connectWS, 2000);
  };

  ws.onerror = () => {
    wsReady = false;
    setConnectionState(false);
  };

  ws.onmessage = (ev) => {
    try {
      const event = JSON.parse(ev.data);
      handleServerEvent(event);
    } catch (e) {
      console.warn("Bad WS message", e);
    }
  };
}

function setConnectionState(connected) {
  const dot = document.getElementById("connection-dot");
  const label = document.getElementById("connection-label");
  dot.className = "status-dot " + (connected ? "connected" : "disconnected");
  label.textContent = connected ? "Live" : "Disconnected";
}

function sendWS(obj) {
  if (ws && ws.readyState === WebSocket.OPEN) {
    ws.send(JSON.stringify(obj));
  }
}

// ─── Server Event Handler ─────────────────────────────────────────────────────
function handleServerEvent(event) {
  // Queue all events so animations play sequentially
  animQueue.push(event);
  drainQueue();
}

function drainQueue() {
  if (animating || animQueue.length === 0) return;
  animating = true;

  const event = animQueue.shift();
  processEvent(event).then(() => {
    // Small delay between steps
    setTimeout(() => {
      animating = false;
      drainQueue();
    }, STEP_DELAY * 0.3);
  });
}

async function processEvent(event) {
  const { type } = event;

  if (type === "snapshot") {
    // Full tree state update
    treeRoot = event.tree;
    if (event.op) {
      const opName = event.op === "insert" ? "Insert" : "Delete";
      setComplexity(opName, "O(log n)", "O(n)");
      if (event.op === "insert") {
        addLog(
          "insert",
          "↑",
          `Inserted "${event.key}"`,
          `O(log n) — tree rebalanced`,
        );
      } else if (event.op === "delete") {
        addLog(
          "delete",
          "↓",
          `Deleted "${event.key}"`,
          `O(log n) — tree rebalanced`,
        );
      }
      totalOps++;
    }
    updateStats();
    await renderTree(treeRoot, null, null);
    hideToast();
  } else if (type === "node_inserted") {
    showToast("insert", `Inserting "${event.key}"…`);
    setComplexity("Insert", "O(log n)", "O(n)");
  } else if (type === "node_deleting") {
    showToast("delete", `Deleting "${event.key}"…`);
    setComplexity("Delete", "O(log n)", "O(n)");
  } else if (type === "rotation") {
    showToast("rotate", `${event.rotation} Rotation on "${event.pivot}"`);
    setComplexity("Rotation", "O(1)", "O(1)");
    addLog(
      "rotate",
      "↻",
      `${event.rotation} Rotation`,
      `Pivot: ${event.pivot}`,
    );
    totalRotations++;
    document.getElementById("stat-rotations").textContent = totalRotations;
  } else if (type === "search_result") {
    highlightKey = event.found ? event.key : null;
    if (event.found) {
      addLog("search", "✦", `Found "${event.key}"`, `O(log n)`);
      showToast("found", `Found "${event.key}"!`);
    } else {
      addLog("search", "✗", `"${event.key}" not found`, `O(log n) — exhausted`);
      showToast("notfound", `"${event.key}" not found`);
    }
    setComplexity("Search", "O(log n)", "O(1)");
    if (treeRoot) await renderTree(treeRoot, highlightKey, null);
    setTimeout(() => {
      highlightKey = null;
      if (treeRoot) renderTree(treeRoot, null, null);
    }, 2000);
  }
}

// ─── D3 Tree Rendering ────────────────────────────────────────────────────────
function renderTree(root, foundKey, deletingKey) {
  return new Promise((resolve) => {
    const emptyEl = document.getElementById("tree-empty");
    const titleEl = document.getElementById("tree-title");

    if (!root) {
      emptyEl.classList.remove("hidden");
      linkLayer.selectAll("*").remove();
      nodeLayer.selectAll("*").remove();
      titleEl.textContent = "AVL Tree (empty)";
      resolve();
      return;
    }

    emptyEl.classList.add("hidden");

    // Convert our JS object into a D3 hierarchy
    const hierarchyData = d3.hierarchy(root, (d) => {
      const children = [];
      if (d.left) children.push(d.left);
      if (d.right) children.push(d.right);
      // D3 needs to know left/right for layout
      if (children.length === 0) return null;
      return children;
    });

    // Compute layout
    const W = svgEl.clientWidth || 800;
    const H = svgEl.clientHeight || 600;

    // Custom tree layout that respects left/right structure
    const treeData = layoutAVL(root, W, H);

    const titleCount = countNodes(root);
    titleEl.textContent = `AVL Tree (${titleCount} node${titleCount !== 1 ? "s" : ""})`;

    // ─ Links ─
    const links = treeData.links;

    const linkSel = linkLayer
      .selectAll("path.link")
      .data(links, (d) => `${d.source.key}-${d.target.key}`);

    // Exit
    linkSel
      .exit()
      .transition()
      .duration(ANIM_DURATION)
      .style("opacity", 0)
      .remove();

    // Enter
    const linkEnter = linkSel
      .enter()
      .append("path")
      .attr("class", "link")
      .style("opacity", 0)
      .attr("d", (d) => straightLink(d.source, d.target));

    // Update + Enter merge
    linkSel
      .merge(linkEnter)
      .transition()
      .duration(ANIM_DURATION)
      .ease(d3.easeCubicInOut)
      .style("opacity", 1)
      .attr("class", (d) => {
        const isRotating = false;
        return `link${isRotating ? " rotating" : ""}`;
      })
      .attr("d", (d) => straightLink(d.source, d.target));

    // ─ Nodes ─
    const nodes = treeData.nodes;

    const nodeSel = nodeLayer.selectAll("g.node").data(nodes, (d) => d.key);

    // Exit
    nodeSel
      .exit()
      .transition()
      .duration(ANIM_DURATION * 0.7)
      .style("opacity", 0)
      .attr("transform", (d) => `translate(${d.x},${d.y}) scale(0)`)
      .remove();

    // Enter
    const nodeEnter = nodeSel
      .enter()
      .append("g")
      .attr("class", (d) => `node ${nodeState(d.key, foundKey, deletingKey)}`)
      .attr("transform", (d) => `translate(${d.x},${d.y}) scale(0)`)
      .style("opacity", 0)
      .style("cursor", "pointer")
      .on("mouseover", (event, d) => showTooltip(event, d))
      .on("mousemove", (event) => moveTooltip(event))
      .on("mouseout", () => hideTooltip())
      .on("click", (event, d) => {
        document.getElementById("key-input").value = d.key;
      });

    nodeEnter.append("circle").attr("r", NODE_RADIUS);

    nodeEnter
      .append("text")
      .attr("class", "node-label")
      .attr("dy", "0.1em")
      .text((d) => truncateKey(d.key));

    // Balance factor badge
    nodeEnter
      .append("text")
      .attr("class", "bf-badge")
      .attr("dy", NODE_RADIUS + 12)
      .attr("fill", (d) => bfColor(d.bf))
      .text((d) => `bf:${d.bf >= 0 ? "+" : ""}${d.bf}`);

    // Merge & Update
    const nodeUpdate = nodeSel.merge(nodeEnter);

    nodeUpdate
      .transition()
      .duration(ANIM_DURATION)
      .ease(d3.easeBackOut.overshoot(1.2))
      .style("opacity", 1)
      .attr("transform", (d) => `translate(${d.x},${d.y}) scale(1)`)
      .attr("class", (d) => `node ${nodeState(d.key, foundKey, deletingKey)}`);

    nodeUpdate.select("text.node-label").text((d) => truncateKey(d.key));

    nodeUpdate
      .select("text.bf-badge")
      .attr("fill", (d) => bfColor(d.bf))
      .text((d) => `bf:${d.bf >= 0 ? "+" : ""}${d.bf}`);

    setTimeout(resolve, ANIM_DURATION + 50);
  });
}

// ─── Custom AVL Tree Layout ───────────────────────────────────────────────────
function layoutAVL(root, W, H) {
  const nodes = [];
  const links = [];

  // Assign x positions bottom-up based on subtree widths
  function subtreeWidth(node) {
    if (!node) return 0;
    const lw = subtreeWidth(node.left);
    const rw = subtreeWidth(node.right);
    return Math.max(1, lw + rw) * (NODE_RADIUS * 2.8);
  }

  // Assign (x, y) positions
  function assignPositions(node, x, y, depth) {
    if (!node) return;

    const lw = subtreeWidth(node.left);
    const rw = subtreeWidth(node.right);
    const totalW = Math.max(1, lw + rw) * (NODE_RADIUS * 2.8);

    const bf = getBalanceFactor(node);

    nodes.push({ key: node.key, x, y, height: node.height, bf, depth });

    if (node.left) {
      const lx = x - (rw > 0 ? totalW / 4 : NODE_RADIUS * 3);
      links.push({
        source: { key: node.key, x, y },
        target: { key: node.left.key, x: lx, y: y + LEVEL_HEIGHT },
      });
      assignPositions(node.left, lx, y + LEVEL_HEIGHT, depth + 1);
    }
    if (node.right) {
      const rx = x + (lw > 0 ? totalW / 4 : NODE_RADIUS * 3);
      links.push({
        source: { key: node.key, x, y },
        target: { key: node.right.key, x: rx, y: y + LEVEL_HEIGHT },
      });
      assignPositions(node.right, rx, y + LEVEL_HEIGHT, depth + 1);
    }
  }

  // Better layout using in-order index
  const ordered = [];
  function inorder(node) {
    if (!node) return;
    inorder(node.left);
    ordered.push(node);
    inorder(node.right);
  }
  inorder(root);

  const n = ordered.length;
  const usableW = W - NODE_RADIUS * 4;
  const startX = NODE_RADIUS * 2;

  const posMap = new Map();
  ordered.forEach((node, i) => {
    posMap.set(node.key, startX + (i / Math.max(1, n - 1)) * usableW);
  });

  // Assign y based on depth using BFS
  function bfsLayout(node, y, depth) {
    if (!node) return;
    const x = posMap.get(node.key) || W / 2;
    posMap.set(node.key + "_y", y);
    posMap.set(node.key + "_depth", depth);
    bfsLayout(node.left, y + LEVEL_HEIGHT, depth + 1);
    bfsLayout(node.right, y + LEVEL_HEIGHT, depth + 1);
  }

  const treeH = treeHeight(root);
  const startY = Math.min(60, (H - treeH * LEVEL_HEIGHT) / 2);
  bfsLayout(root, startY, 0);

  ordered.forEach((node) => {
    const x = posMap.get(node.key) || W / 2;
    const y = posMap.get(node.key + "_y") || 60;
    const bf = getBalanceFactor(node);
    nodes.push({ key: node.key, x, y, height: node.height, bf });
  });

  // Build links
  function buildLinks(node) {
    if (!node) return;
    const sx = posMap.get(node.key);
    const sy = posMap.get(node.key + "_y");
    if (node.left) {
      const tx = posMap.get(node.left.key);
      const ty = posMap.get(node.left.key + "_y");
      links.push({
        source: { key: node.key, x: sx, y: sy },
        target: { key: node.left.key, x: tx, y: ty },
      });
      buildLinks(node.left);
    }
    if (node.right) {
      const tx = posMap.get(node.right.key);
      const ty = posMap.get(node.right.key + "_y");
      links.push({
        source: { key: node.key, x: sx, y: sy },
        target: { key: node.right.key, x: tx, y: ty },
      });
      buildLinks(node.right);
    }
  }
  buildLinks(root);

  return { nodes, links };
}

// ─── Helpers ──────────────────────────────────────────────────────────────────
function straightLink(source, target) {
  const dx = target.x - source.x;
  const dy = target.y - source.y;
  const angle = Math.atan2(dy, dx);
  const sx = source.x + Math.cos(angle) * NODE_RADIUS;
  const sy = source.y + Math.sin(angle) * NODE_RADIUS;
  const tx = target.x - Math.cos(angle) * NODE_RADIUS;
  const ty = target.y - Math.sin(angle) * NODE_RADIUS;
  // Bezier curve for a more organic look
  const mx = (sx + tx) / 2;
  const my = (sy + ty) / 2 - Math.abs(dx) * 0.1;
  return `M${sx},${sy} Q${mx},${my} ${tx},${ty}`;
}

function nodeState(key, foundKey, deletingKey) {
  if (foundKey && key === foundKey) return "state-found";
  if (deletingKey && key === deletingKey) return "state-deleting";
  return "state-normal";
}

function truncateKey(key) {
  if (!key) return "";
  if (key.length <= 5) return key;
  return key.substring(0, 4) + "…";
}

function bfColor(bf) {
  if (bf === 0) return "rgba(255,255,255,0.4)";
  if (Math.abs(bf) === 1) return "rgba(79,158,255,0.7)";
  return "rgba(248,113,113,0.9)"; // |bf| >= 2 is unbalanced
}

function getBalanceFactor(node) {
  if (!node) return 0;
  const lh = node.left ? node.left.height : 0;
  const rh = node.right ? node.right.height : 0;
  return lh - rh;
}

function treeHeight(node) {
  if (!node) return 0;
  return 1 + Math.max(treeHeight(node.left), treeHeight(node.right));
}

function countNodes(node) {
  if (!node) return 0;
  return 1 + countNodes(node.left) + countNodes(node.right);
}

function updateStats() {
  const n = countNodes(treeRoot);
  const h = treeHeight(treeRoot);
  document.getElementById("stat-nodes").textContent = n;
  document.getElementById("stat-height").textContent = h;
  document.getElementById("stat-ops").textContent =
    ++totalOps < 1 ? 0 : totalOps;
}

// ─── Complexity Panel ─────────────────────────────────────────────────────────
function setComplexity(opName, time, space) {
  document.getElementById("current-op-name").textContent = opName;
  document.getElementById("bigo-time").textContent = time;
  document.getElementById("bigo-space").textContent = space;
}

// ─── Toast ────────────────────────────────────────────────────────────────────
const toastEl = document.getElementById("op-toast");
const toastIcon = document.getElementById("op-toast-icon");
const toastText = document.getElementById("op-toast-text");

const toastStyles = {
  insert: { icon: "↑", bg: "rgba(52,211,153,0.2)", color: "#34d399" },
  delete: { icon: "↓", bg: "rgba(248,113,113,0.2)", color: "#f87171" },
  rotate: { icon: "↻", bg: "rgba(251,191,36,0.2)", color: "#fbbf24" },
  found: { icon: "✦", bg: "rgba(34,211,238,0.2)", color: "#22d3ee" },
  notfound: {
    icon: "✗",
    bg: "rgba(255,255,255,0.1)",
    color: "rgba(255,255,255,0.5)",
  },
};

function showToast(type, text) {
  const style = toastStyles[type] || toastStyles.insert;
  toastIcon.textContent = style.icon;
  toastIcon.style.background = style.bg;
  toastIcon.style.color = style.color;
  toastText.textContent = text;
  toastEl.classList.remove("hidden");
}

function hideToast() {
  setTimeout(() => toastEl.classList.add("hidden"), 1200);
}

// ─── Tooltip ──────────────────────────────────────────────────────────────────
function showTooltip(event, d) {
  tooltip.innerHTML = `
    <div style="font-weight:600;margin-bottom:4px;">${d.key}</div>
    <div style="color:rgba(255,255,255,0.5);font-size:10px;">Height: ${d.height} &nbsp;|&nbsp; BF: ${d.bf >= 0 ? "+" : ""}${d.bf}</div>
  `;
  tooltip.classList.add("visible");
  moveTooltip(event);
}

function moveTooltip(event) {
  tooltip.style.left = event.clientX + 12 + "px";
  tooltip.style.top = event.clientY - 36 + "px";
}

function hideTooltip() {
  tooltip.classList.remove("visible");
}

// ─── Activity Log ─────────────────────────────────────────────────────────────
const logEl = document.getElementById("activity-log");

function addLog(type, icon, main, detail) {
  const now = new Date();
  const timeStr = `${now.getHours().toString().padStart(2, "0")}:${now.getMinutes().toString().padStart(2, "0")}:${now.getSeconds().toString().padStart(2, "0")}`;

  const entry = document.createElement("div");
  entry.className = "log-entry";
  entry.innerHTML = `
    <div class="log-icon ${type}">${icon}</div>
    <div class="log-text">
      <div class="log-main">${main}</div>
      <div class="log-detail">${detail}</div>
    </div>
    <div class="log-time">${timeStr}</div>
  `;

  // Prepend so newest is on top
  logEl.insertBefore(entry, logEl.firstChild);

  // Keep max 100 entries
  while (logEl.children.length > 100) {
    logEl.removeChild(logEl.lastChild);
  }
}

function clearLog() {
  logEl.innerHTML = "";
}

// ─── User Actions ─────────────────────────────────────────────────────────────
function insertKey() {
  const input = document.getElementById("key-input");
  const key = input.value.trim();
  if (!key) {
    flashInput();
    return;
  }

  sendWS({ type: "insert", key });
  input.value = "";
  input.focus();
}

function deleteKey() {
  const input = document.getElementById("key-input");
  const key = input.value.trim();
  if (!key) {
    flashInput();
    return;
  }

  sendWS({ type: "delete", key });
  input.value = "";
  input.focus();
}

function searchKey() {
  const input = document.getElementById("key-input");
  const key = input.value.trim();
  if (!key) {
    flashInput();
    return;
  }

  // Local search simulation
  const found = searchInTree(treeRoot, key);
  handleServerEvent({ type: "search_result", key, found });
  setComplexity("Search", "O(log n)", "O(1)");
  totalOps++;
  document.getElementById("stat-ops").textContent = totalOps;
}

function clearTree() {
  treeRoot = null;
  animQueue = [];
  animating = false;
  renderTree(null, null, null);
  addLog("system", "↺", "Tree reset", "Visualizer cleared");
  sendWS({ type: "reset" });
}

function searchInTree(node, key) {
  if (!node) return false;
  if (node.key === key) return true;
  if (key < node.key) return searchInTree(node.left, key);
  return searchInTree(node.right, key);
}

function flashInput() {
  const input = document.getElementById("key-input");
  input.style.borderColor = "var(--red)";
  input.style.boxShadow = "0 0 0 3px rgba(248,113,113,0.2)";
  setTimeout(() => {
    input.style.borderColor = "";
    input.style.boxShadow = "";
  }, 600);
}

// Enter key submits insert
document.getElementById("key-input").addEventListener("keydown", (e) => {
  if (e.key === "Enter") insertKey();
  if (e.key === "Delete" && e.ctrlKey) deleteKey();
});

// ─── Resize handling ──────────────────────────────────────────────────────────
let resizeTimer;
window.addEventListener("resize", () => {
  clearTimeout(resizeTimer);
  resizeTimer = setTimeout(() => {
    if (treeRoot) renderTree(treeRoot, highlightKey, null);
  }, 150);
});

// ─── Boot ─────────────────────────────────────────────────────────────────────
addLog("system", "◎", "Visualizer started", "Connecting to backend…");
connectWS();

// Periodically update stats display (in case they drift)
setInterval(() => {
  document.getElementById("stat-nodes").textContent = countNodes(treeRoot);
  document.getElementById("stat-height").textContent = treeHeight(treeRoot);
  document.getElementById("stat-rotations").textContent = totalRotations;
  document.getElementById("stat-ops").textContent = totalOps;
}, 2000);
