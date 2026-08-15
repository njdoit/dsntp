// Command server is the Time Control Server process entry (M4 skeleton).
//
// Red lines (DRS-002):
//   - MUST NOT join P2P consensus
//   - MUST NOT rewrite Agent consensus Tc
//   - Agent must keep consensus when this process is down
package main

import (
	"flag"
	"log"
	"net/http"
	"os"
	"os/signal"
	"syscall"

	"dsntp/ctl/internal/alert"
	"dsntp/ctl/internal/config"
	"dsntp/ctl/internal/httpapi"
	"dsntp/ctl/internal/ingest"
)

func main() {
	httpAddr := flag.String("http", envOr("DSNTP_CTL_HTTP", ":8080"), "HTTP listen address")
	grpcAddr := flag.String("grpc", envOr("DSNTP_CTL_GRPC", ":50051"), "gRPC listen address (placeholder)")
	flag.Parse()

	store := ingest.NewStore()
	cfg := config.NewManager()
	alerter := alert.NewEngine(store)

	mux := httpapi.NewMux(store, cfg, alerter)

	go func() {
		log.Printf("dsntp-ctl HTTP listening on %s (IF-CTL)", *httpAddr)
		if err := http.ListenAndServe(*httpAddr, mux); err != nil {
			log.Fatalf("http: %v", err)
		}
	}()

	// gRPC ReportStream: wire api/agent.proto in M4 implementation.
	log.Printf("dsntp-ctl gRPC placeholder on %s (ReportStream TODO)", *grpcAddr)

	ch := make(chan os.Signal, 1)
	signal.Notify(ch, syscall.SIGINT, syscall.SIGTERM)
	<-ch
	log.Printf("dsntp-ctl shutting down")
}

func envOr(k, def string) string {
	if v := os.Getenv(k); v != "" {
		return v
	}
	return def
}
