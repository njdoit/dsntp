package config

import (
	"sync"
	"time"
)

// VersionedConfig is a publishable control-plane config blob.
// Agents apply locally; Server never writes Tc.
type VersionedConfig struct {
	Version   string    `json:"version"`
	Payload   string    `json:"payload_json"`
	Published time.Time `json:"published_at"`
}

type Manager struct {
	mu      sync.RWMutex
	current VersionedConfig
	acks    map[uint32]string // node_id -> last acked version
}

func NewManager() *Manager {
	return &Manager{acks: make(map[uint32]string)}
}

func (m *Manager) Publish(version, payload string) VersionedConfig {
	m.mu.Lock()
	defer m.mu.Unlock()
	m.current = VersionedConfig{
		Version:   version,
		Payload:   payload,
		Published: time.Now().UTC(),
	}
	return m.current
}

func (m *Manager) Current() VersionedConfig {
	m.mu.RLock()
	defer m.mu.RUnlock()
	return m.current
}

func (m *Manager) Ack(nodeID uint32, version string, applied bool) {
	if !applied {
		return
	}
	m.mu.Lock()
	defer m.mu.Unlock()
	m.acks[nodeID] = version
}
