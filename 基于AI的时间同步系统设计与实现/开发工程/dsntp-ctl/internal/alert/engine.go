package alert

import (
	"time"

	"dsntp/ctl/internal/ingest"
)

// Engine evaluates default OPS thresholds (NFR-OPS-002 skeleton).
type Engine struct {
	store *ingest.Store
}

func NewEngine(store *ingest.Store) *Engine {
	return &Engine{store: store}
}

func (e *Engine) Evaluate() []ingest.Event {
	now := time.Now().UTC()
	var out []ingest.Event
	for _, r := range e.store.List() {
		if r.MaxPeerOffsetMS > 5 {
			out = append(out, ingest.Event{
				NodeID: r.NodeID, At: now, Severity: "critical",
				Code: "PEER_OFFSET_HIGH", Message: "max_peer_offset_ms > 5",
			})
		} else if r.MaxPeerOffsetMS > 1 {
			out = append(out, ingest.Event{
				NodeID: r.NodeID, At: now, Severity: "warn",
				Code: "PEER_OFFSET_WARN", Message: "max_peer_offset_ms > 1",
			})
		}
		if now.Sub(r.ReceivedAt) > 30*time.Second {
			out = append(out, ingest.Event{
				NodeID: r.NodeID, At: now, Severity: "warn",
				Code: "HEARTBEAT_MISS", Message: "no report > 30s",
			})
		}
	}
	return out
}
