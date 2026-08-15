package ingest

import (
	"sync"
	"time"
)

// TimeReport is the in-memory IF-CTL report snapshot (aligns with agent.proto).
type TimeReport struct {
	NodeID           uint32    `json:"node_id"`
	Seq              uint64    `json:"seq"`
	ReportedAtNS     uint64    `json:"reported_at_ns"`
	ConfigVersion    string    `json:"config_version"`
	MonotonicNS      uint64    `json:"monotonic_ns"`
	SyncedNS         uint64    `json:"synced_ns"`
	ConsensusTC      uint64    `json:"consensus_tc"` // read-only observation
	Round            uint64    `json:"round"`
	FSMState         string    `json:"fsm_state"`
	MaxPeerOffsetMS  float64   `json:"max_peer_offset_ms"`
	RTTAvgMS         float64   `json:"rtt_avg_ms"`
	ReceivedAt       time.Time `json:"received_at"`
}

// Store holds latest reports per node. Does not compute or rewrite Tc.
type Store struct {
	mu      sync.RWMutex
	latest  map[uint32]TimeReport
	events  []Event
}

type Event struct {
	NodeID   uint32    `json:"node_id"`
	At       time.Time `json:"at"`
	Severity string    `json:"severity"`
	Code     string    `json:"code"`
	Message  string    `json:"message"`
}

func NewStore() *Store {
	return &Store{
		latest: make(map[uint32]TimeReport),
		events: make([]Event, 0, 64),
	}
}

func (s *Store) Upsert(r TimeReport) {
	s.mu.Lock()
	defer s.mu.Unlock()
	r.ReceivedAt = time.Now().UTC()
	s.latest[r.NodeID] = r
}

func (s *Store) List() []TimeReport {
	s.mu.RLock()
	defer s.mu.RUnlock()
	out := make([]TimeReport, 0, len(s.latest))
	for _, v := range s.latest {
		out = append(out, v)
	}
	return out
}

func (s *Store) Get(id uint32) (TimeReport, bool) {
	s.mu.RLock()
	defer s.mu.RUnlock()
	v, ok := s.latest[id]
	return v, ok
}

func (s *Store) AddEvent(e Event) {
	s.mu.Lock()
	defer s.mu.Unlock()
	if e.At.IsZero() {
		e.At = time.Now().UTC()
	}
	s.events = append(s.events, e)
}

func (s *Store) Events() []Event {
	s.mu.RLock()
	defer s.mu.RUnlock()
	out := make([]Event, len(s.events))
	copy(out, s.events)
	return out
}
