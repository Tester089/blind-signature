param(
    [Parameter(Mandatory = $true)]
    [string]$Username,
    [Parameter(Mandatory = $true)]
    [string]$Password
)

if (-not $env:BS_ADMIN_DB_CONN) {
    Write-Error "BS_ADMIN_DB_CONN is not set. Example: set BS_ADMIN_DB_CONN=postgresql://admin:pass@localhost:5432/dbname"
    exit 1
}

$psql = $env:PSQL_BIN
if (-not $psql) { $psql = "psql" }

$u = $Username.Replace("'", "''")
$p = $Password.Replace("'", "''")

$sql = "CREATE EXTENSION IF NOT EXISTS pgcrypto; INSERT INTO users(username, password_hash) VALUES ('$u', crypt('$p', gen_salt('bf'))) ON CONFLICT (username) DO UPDATE SET password_hash = EXCLUDED.password_hash;"

& $psql $env:BS_ADMIN_DB_CONN -v ON_ERROR_STOP=1 -c $sql
