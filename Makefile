CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra
SRC      := src/main.cpp
DATADIR  := src

# ─── Default build (no visualizer) ──────────────────────────────────────────
all: morphology-engine

morphology-engine: $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^

# ─── Visualizer build (emits JSON events to /tmp/avl_events.jsonl) ───────────
visualizer: src/main.cpp
	$(CXX) $(CXXFLAGS) -DAVL_VISUALIZER -o morphology-engine-vis $^
	@echo ""
	@echo "  ✔ Built morphology-engine-vis with visualizer support"
	@echo ""
	@echo "  To run:"
	@echo "    1. cd visualizer && npm install && node server.js &"
	@echo "    2. open http://localhost:3000"
	@echo "    3. ./morphology-engine-vis"
	@echo ""

# ─── Install node deps ───────────────────────────────────────────────────────
install-vis:
	cd visualizer && npm install

# ─── Start visualizer server in background ──────────────────────────────────
start-server:
	cd visualizer && node server.js

# ─── Clean ───────────────────────────────────────────────────────────────────
clean:
	rm -f morphology-engine morphology-engine-vis /tmp/avl_events.jsonl /tmp/avl_cmd.txt

.PHONY: all visualizer install-vis start-server clean
