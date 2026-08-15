package httpapi

import (
	"encoding/json"
	"net/http"
	"strconv"
	"strings"

	"dsntp/ctl/internal/alert"
	"dsntp/ctl/internal/config"
	"dsntp/ctl/internal/ingest"
)

// NewMux wires IF-CTL HTTP stubs (DRS-002 §9.2).
func NewMux(store *ingest.Store, cfg *config.Manager, alerter *alert.Engine) http.Handler {
	mux := http.NewServeMux()

	mux.HandleFunc("/api/v1/health", func(w http.ResponseWriter, _ *http.Request) {
		writeJSON(w, http.StatusOK, map[string]string{"status": "ok", "service": "dsntp-ctl"})
	})

	// IF-CTL-004
	mux.HandleFunc("/api/v1/nodes", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodGet {
			http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
			return
		}
		writeJSON(w, http.StatusOK, store.List())
	})

	// IF-CTL-005 (skeleton: returns latest snapshot only)
	mux.HandleFunc("/api/v1/nodes/", func(w http.ResponseWriter, r *http.Request) {
		path := strings.TrimPrefix(r.URL.Path, "/api/v1/nodes/")
		parts := strings.Split(path, "/")
		if len(parts) == 0 || parts[0] == "" {
			http.NotFound(w, r)
			return
		}
		id64, err := strconv.ParseUint(parts[0], 10, 32)
		if err != nil {
			http.Error(w, "bad node id", http.StatusBadRequest)
			return
		}
		id := uint32(id64)

		if len(parts) >= 2 && parts[1] == "metrics" && r.Method == http.MethodGet {
			rep, ok := store.Get(id)
			if !ok {
				http.NotFound(w, r)
				return
			}
			writeJSON(w, http.StatusOK, rep)
			return
		}

		// IF-CTL-007 Ext_Sync: accept and queue (no Tc rewrite)
		if len(parts) >= 2 && parts[1] == "ext-sync" && r.Method == http.MethodPost {
			store.AddEvent(ingest.Event{
				NodeID: id, Severity: "info", Code: "EXT_SYNC_QUEUED",
				Message: "ext-sync command accepted (delivery TODO)",
			})
			writeJSON(w, http.StatusAccepted, map[string]string{"status": "queued"})
			return
		}

		http.NotFound(w, r)
	})

	// IF-CTL-006
	mux.HandleFunc("/api/v1/config/publish", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodPost {
			http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
			return
		}
		var body struct {
			Version string `json:"version"`
			Payload string `json:"payload_json"`
		}
		if err := json.NewDecoder(r.Body).Decode(&body); err != nil {
			http.Error(w, "bad json", http.StatusBadRequest)
			return
		}
		pub := cfg.Publish(body.Version, body.Payload)
		writeJSON(w, http.StatusAccepted, pub)
	})

	// IF-CTL-008
	mux.HandleFunc("/api/v1/events", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodGet {
			http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
			return
		}
		_ = alerter // evaluate on demand in later M4
		writeJSON(w, http.StatusOK, store.Events())
	})

	// Agent HTTPS JSON ingest (P0 equivalent to gRPC ReportStream)
	mux.HandleFunc("/api/v1/ingest/report", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodPost {
			http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
			return
		}
		var rep ingest.TimeReport
		if err := json.NewDecoder(r.Body).Decode(&rep); err != nil {
			http.Error(w, "bad json", http.StatusBadRequest)
			return
		}
		store.Upsert(rep)
		writeJSON(w, http.StatusOK, map[string]string{"status": "accepted"})
	})

	return mux
}

func writeJSON(w http.ResponseWriter, code int, v any) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(code)
	_ = json.NewEncoder(w).Encode(v)
}
