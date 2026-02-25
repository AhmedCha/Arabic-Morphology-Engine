.PHONY: all clean run visualizer install-vis start-server build/ArabicMorphology

# ─── Default build (no visualizer) ──────────────────────────────────────────
all: build/ArabicMorphology

build/ArabicMorphology:
	mkdir -p build
	cd build && cmake ../src
	$(MAKE) -C build

# ─── Run the application ─────────────────────────────────────────────────────
run: all
	cd build && ./ArabicMorphology

# ─── Visualizer build (emits JSON events to /tmp/avl_events.jsonl) ───────────
visualizer:
	mkdir -p build
	cd build && cmake -DCMAKE_CXX_FLAGS="-DAVL_VISUALIZER" ../src
	$(MAKE) -C build
	@echo ""
	@echo "  ✔ Built ArabicMorphology with visualizer support"
	@echo ""
	@echo "  To run:"
	@echo "    1. make start-server &"
	@echo "    2. open http://localhost:3000"
	@echo "    3. make run"
	@echo ""

# ─── Install node deps ───────────────────────────────────────────────────────
install-vis:
	cd visualizer && npm install

# ─── Start visualizer server in background ──────────────────────────────────
start-server:
	cd visualizer && node server.js

# ─── Clean ───────────────────────────────────────────────────────────────────
clean:
	rm -rf build /tmp/avl_events.jsonl /tmp/avl_cmd.txt
