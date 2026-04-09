"""
Built-in Sigma-inspired correlation rules.

Each rule contains:
  - id          Unique identifier (used in notable event IDs)
  - name        Human-readable title
  - description What this rule detects
  - severity    info | low | medium | high | critical
  - mitre       MITRE ATT&CK technique IDs
  - query       SPL query; results indicate a detection hit
  - condition   How to interpret search results:
                  {"count_gt": N}   → fire if total_count > N
                  {"count_ge": N}   → fire if total_count >= N
  - risk_score  0-100 numeric score for RBA

Rules are intentionally broad so they trigger on the test-ingest.sh sample data.
"""

from dataclasses import dataclass, field


@dataclass
class BuiltinRule:
    id: str
    name: str
    description: str
    severity: str
    query: str
    condition: dict
    risk_score: int = 50
    mitre: list[str] = field(default_factory=list)


BUILTIN_RULES: list[BuiltinRule] = [

    # ── Brute Force ───────────────────────────────────────────────────────────
    BuiltinRule(
        id="BF-001",
        name="Multiple Failed Logins — Potential Brute Force",
        description=(
            "Three or more failed authentication events within the search window "
            "from a single source. Indicates a password-spray or brute-force attempt."
        ),
        severity="high",
        query=(
            "search index=main (EventCode=4625 OR \"Failed password\" OR \"failed login\" OR \"authentication failure\") "
            "| stats count by host "
            "| where count >= 3"
        ),
        condition={"count_gt": 0},
        risk_score=70,
        mitre=["T1110", "T1078"],
    ),

    # ── Successful Login After Failures ───────────────────────────────────────
    BuiltinRule(
        id="BF-002",
        name="Successful Login Following Failed Attempts",
        description=(
            "A successful authentication (EventCode 4624 or SSH Accepted) was observed "
            "alongside prior failures — may indicate a successful brute-force."
        ),
        severity="critical",
        query=(
            "search index=main (EventCode=4624 OR \"Accepted publickey\" OR \"Accepted password\") "
            "| stats count by host"
        ),
        condition={"count_gt": 0},
        risk_score=85,
        mitre=["T1110", "T1078.001"],
    ),

    # ── Suspicious Process Execution ─────────────────────────────────────────
    BuiltinRule(
        id="PE-001",
        name="Suspicious Process Creation — cmd.exe / PowerShell",
        description=(
            "A new process (EventCode 4688) was created with cmd.exe or powershell. "
            "May indicate lateral movement or post-exploitation activity."
        ),
        severity="medium",
        query=(
            "search index=main EventCode=4688 "
            "(\"cmd.exe\" OR \"powershell\" OR \"wscript\" OR \"cscript\") "
            "| stats count by host"
        ),
        condition={"count_gt": 0},
        risk_score=55,
        mitre=["T1059.001", "T1059.003"],
    ),

    # ── Privilege Escalation via sudo ─────────────────────────────────────────
    BuiltinRule(
        id="PE-002",
        name="Privilege Escalation via sudo",
        description=(
            "A user executed a command as root via sudo. "
            "Expected for admins; unexpected for regular service accounts."
        ),
        severity="medium",
        query=(
            "search index=main sudo "
            "| stats count by host"
        ),
        condition={"count_gt": 0},
        risk_score=45,
        mitre=["T1548.003"],
    ),

    # ── Threat Detected by Firewall ───────────────────────────────────────────
    BuiltinRule(
        id="FW-001",
        name="Firewall Threat Alert — Spyware or Malware Detected",
        description=(
            "A THREAT event with severity=critical was logged by the perimeter firewall. "
            "Indicates an active C2 communication or malware beacon."
        ),
        severity="critical",
        query=(
            "search index=main (THREAT OR threat_type) (critical OR high) "
            "| stats count by host"
        ),
        condition={"count_gt": 0},
        risk_score=90,
        mitre=["T1071", "T1571"],
    ),

    # ── External SSH Blocked ──────────────────────────────────────────────────
    BuiltinRule(
        id="FW-002",
        name="Blocked External SSH Attempt",
        description=(
            "Firewall policy denied an SSH connection originating from an external "
            "IP address. May indicate reconnaissance or access attempt."
        ),
        severity="low",
        query=(
            "search index=main (\"block-external-ssh\" OR (deny port=22)) "
            "| stats count by host"
        ),
        condition={"count_gt": 0},
        risk_score=30,
        mitre=["T1021.004"],
    ),

    # ── Web Directory Traversal ───────────────────────────────────────────────
    BuiltinRule(
        id="WEB-001",
        name="Web Directory Traversal Attempt",
        description=(
            "An HTTP request contained path traversal sequences (../). "
            "Typical of automated scanners or exploitation attempts targeting "
            "sensitive files such as /etc/passwd."
        ),
        severity="high",
        query=(
            "search index=main (\"../\" OR \"%2e%2e\" OR \"%2f\") "
            "| stats count by host"
        ),
        condition={"count_gt": 0},
        risk_score=65,
        mitre=["T1083", "T1190"],
    ),

    # ── Unauthorized Admin Access Attempt ─────────────────────────────────────
    BuiltinRule(
        id="WEB-002",
        name="Unauthorized Access to Admin Endpoint",
        description=(
            "HTTP 403 Forbidden returned for an admin or configuration path. "
            "May indicate a probing attempt against management interfaces."
        ),
        severity="medium",
        query=(
            "search index=main (\"/admin\" OR \"/config\" OR \"/management\") "
            "(403 OR 401) "
            "| stats count by host"
        ),
        condition={"count_gt": 0},
        risk_score=50,
        mitre=["T1078", "T1190"],
    ),

    # ── Outbound C2 Communication ─────────────────────────────────────────────
    BuiltinRule(
        id="NET-001",
        name="Outbound Connection to Suspicious Domain",
        description=(
            "A DNS query or HTTPS connection was made to a domain classified as "
            "malicious or suspicious (e.g., evil-domain.xyz). Indicates possible "
            "C2 beaconing or data exfiltration."
        ),
        severity="critical",
        query=(
            "search index=main (\"evil-domain\" OR spyware OR \"block-malicious\") "
            "| stats count by host"
        ),
        condition={"count_gt": 0},
        risk_score=95,
        mitre=["T1071.001", "T1568"],
    ),

    # ── Automated Scanner Detected ────────────────────────────────────────────
    BuiltinRule(
        id="WEB-003",
        name="Automated Web Scanner Detected (Nikto / curl)",
        description=(
            "A web request was made using a known vulnerability scanner or "
            "command-line HTTP client (Nikto, curl, python-requests). "
            "May indicate active reconnaissance."
        ),
        severity="medium",
        query=(
            "search index=main (\"Nikto\" OR \"sqlmap\" OR \"curl/\" OR \"python-requests\") "
            "| stats count by host"
        ),
        condition={"count_gt": 0},
        risk_score=55,
        mitre=["T1595.002"],
    ),
]
