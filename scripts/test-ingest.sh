#!/bin/bash
# Test script: sends sample events directly to the C++ engine HTTP API
# Usage: bash scripts/test-ingest.sh [engine_url]

ENGINE_URL="${1:-http://localhost:8080}"

echo "=== Sentinel SIEM - Test Event Ingestion ==="
echo "Engine: $ENGINE_URL"
echo ""

# Check engine health
echo "Checking engine health..."
curl -s "$ENGINE_URL/api/health" | python3 -m json.tool 2>/dev/null || echo "(install python3 for pretty JSON)"
echo ""

# Ingest Windows Security Events
echo "Sending Windows Security Events..."
curl -s -X POST "$ENGINE_URL/api/ingest" -H "Content-Type: application/json" -d '{
  "index": "main",
  "sourcetype": "WinEventLog:Security",
  "host": "dc-01.corp.local",
  "source": "WinEventLog:Security",
  "events": [
    {"raw": "2024-01-15T10:30:00Z EventCode=4625 An account failed to log on. Subject: Security ID: S-1-0-0 Account Name: - Logon Type: 3 Account For Which Logon Failed: Account Name: admin Target: dc-01.corp.local Source Network Address: 192.168.1.100"},
    {"raw": "2024-01-15T10:30:01Z EventCode=4625 An account failed to log on. Subject: Security ID: S-1-0-0 Account Name: - Logon Type: 3 Account For Which Logon Failed: Account Name: admin Target: dc-01.corp.local Source Network Address: 192.168.1.100"},
    {"raw": "2024-01-15T10:30:02Z EventCode=4625 An account failed to log on. Subject: Security ID: S-1-0-0 Account Name: - Logon Type: 3 Account For Which Logon Failed: Account Name: admin Target: dc-01.corp.local Source Network Address: 192.168.1.100"},
    {"raw": "2024-01-15T10:30:05Z EventCode=4624 An account was successfully logged on. Subject: Security ID: S-1-5-18 Account Name: SYSTEM Logon Type: 3 New Logon: Account Name: admin Target: dc-01.corp.local Source Network Address: 192.168.1.100"},
    {"raw": "2024-01-15T10:31:00Z EventCode=4688 A new process has been created. Creator Subject: Account Name: admin Process Information: New Process Name: C:\\Windows\\System32\\cmd.exe Process Command Line: cmd.exe /c whoami"}
  ]
}'
echo ""

# Ingest Firewall Logs
echo "Sending Firewall Logs..."
curl -s -X POST "$ENGINE_URL/api/ingest" -H "Content-Type: application/json" -d '{
  "index": "main",
  "sourcetype": "pan:traffic",
  "host": "fw-edge-01",
  "source": "pan:traffic",
  "events": [
    {"raw": "2024-01-15T10:30:00Z TRAFFIC,allow,192.168.1.50,10.0.0.5,80,443,tcp,web-browsing,trust,untrust,bytes=15234,packets=42,session_end_reason=aged-out"},
    {"raw": "2024-01-15T10:30:01Z TRAFFIC,deny,10.20.30.40,185.220.101.34,0,443,tcp,ssl,untrust,trust,bytes=0,packets=1,session_end_reason=policy-deny,rule=block-malicious"},
    {"raw": "2024-01-15T10:30:02Z TRAFFIC,allow,192.168.1.100,8.8.8.8,0,53,udp,dns,trust,untrust,bytes=128,packets=2,session_end_reason=aged-out"},
    {"raw": "2024-01-15T10:30:03Z THREAT,alert,192.168.1.75,evil-domain.xyz,0,443,tcp,ssl,trust,untrust,threat_type=spyware,severity=critical,action=alert"},
    {"raw": "2024-01-15T10:30:04Z TRAFFIC,deny,10.99.99.99,192.168.1.10,0,22,tcp,ssh,untrust,trust,bytes=0,packets=3,session_end_reason=policy-deny,rule=block-external-ssh"}
  ]
}'
echo ""

# Ingest Linux Auth Logs
echo "Sending Linux Auth Logs..."
curl -s -X POST "$ENGINE_URL/api/ingest" -H "Content-Type: application/json" -d '{
  "index": "main",
  "sourcetype": "linux_secure",
  "host": "web-server-01",
  "source": "/var/log/auth.log",
  "events": [
    {"raw": "Jan 15 10:30:00 web-server-01 sshd[12345]: Failed password for root from 10.0.0.99 port 22 ssh2"},
    {"raw": "Jan 15 10:30:01 web-server-01 sshd[12346]: Failed password for root from 10.0.0.99 port 22 ssh2"},
    {"raw": "Jan 15 10:30:02 web-server-01 sshd[12347]: Failed password for admin from 10.0.0.99 port 22 ssh2"},
    {"raw": "Jan 15 10:30:03 web-server-01 sshd[12348]: Accepted publickey for deploy from 10.0.1.5 port 22 ssh2"},
    {"raw": "Jan 15 10:30:10 web-server-01 sudo: deploy : TTY=pts/0 ; PWD=/home/deploy ; USER=root ; COMMAND=/bin/systemctl restart nginx"}
  ]
}'
echo ""

# Ingest Application Logs
echo "Sending Application Logs..."
curl -s -X POST "$ENGINE_URL/api/ingest" -H "Content-Type: application/json" -d '{
  "index": "main",
  "sourcetype": "nginx:access",
  "host": "web-server-01",
  "source": "/var/log/nginx/access.log",
  "events": [
    {"raw": "192.168.1.50 - - [15/Jan/2024:10:30:00 +0000] \"GET /api/users HTTP/1.1\" 200 1234 \"-\" \"Mozilla/5.0\""},
    {"raw": "192.168.1.51 - - [15/Jan/2024:10:30:01 +0000] \"POST /api/login HTTP/1.1\" 401 89 \"-\" \"curl/7.88.1\""},
    {"raw": "192.168.1.52 - - [15/Jan/2024:10:30:02 +0000] \"GET /admin/config HTTP/1.1\" 403 0 \"-\" \"python-requests/2.31\""},
    {"raw": "10.0.0.99 - - [15/Jan/2024:10:30:03 +0000] \"GET /../../../etc/passwd HTTP/1.1\" 400 0 \"-\" \"Nikto/2.1.6\""},
    {"raw": "192.168.1.50 - - [15/Jan/2024:10:30:04 +0000] \"GET /api/dashboard HTTP/1.1\" 200 5678 \"-\" \"Mozilla/5.0\""}
  ]
}'
echo ""

# Test search
echo "=== Testing Search ==="
echo ""
echo "1. Search all events:"
curl -s -X POST "$ENGINE_URL/api/search" -H "Content-Type: application/json" -d '{"query": "search index=main"}' | python3 -c "import sys,json; d=json.load(sys.stdin); print(f'  Found {d[\"total_count\"]} events in {d[\"execution_time_ms\"]:.2f}ms')" 2>/dev/null
echo ""

echo "2. Search for failed logins:"
curl -s -X POST "$ENGINE_URL/api/search" -H "Content-Type: application/json" -d '{"query": "search index=main failed"}' | python3 -c "import sys,json; d=json.load(sys.stdin); print(f'  Found {d[\"total_count\"]} events')" 2>/dev/null
echo ""

echo "3. Stats by host:"
curl -s -X POST "$ENGINE_URL/api/search" -H "Content-Type: application/json" -d '{"query": "search index=main | stats count by host"}' | python3 -c "import sys,json; d=json.load(sys.stdin); print(f'  {len(d[\"events\"])} groups'); [print(f'    {e}') for e in d['events']]" 2>/dev/null
echo ""

echo "4. Stats by sourcetype:"
curl -s -X POST "$ENGINE_URL/api/search" -H "Content-Type: application/json" -d '{"query": "search index=main | stats count by sourcetype"}' | python3 -c "import sys,json; d=json.load(sys.stdin); print(f'  {len(d[\"events\"])} groups'); [print(f'    {e}') for e in d['events']]" 2>/dev/null
echo ""

echo "5. Top hosts sorted:"
curl -s -X POST "$ENGINE_URL/api/search" -H "Content-Type: application/json" -d '{"query": "search index=main | stats count by host | sort -count | head 5"}' | python3 -c "import sys,json; d=json.load(sys.stdin); [print(f'    {e}') for e in d['events']]" 2>/dev/null
echo ""

# Check indexes
echo "=== Index Status ==="
curl -s "$ENGINE_URL/api/indexes" | python3 -m json.tool 2>/dev/null
echo ""

echo "=== Done ==="
