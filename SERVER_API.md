# Blind Signatures Server API

## 1) Start server

```powershell
g++ -std=c++20 -O2 -D_WIN32_WINNT=0x0A00 server.cpp -o blind_server.exe -lws2_32
./blind_server.exe
```

Server URL: `http://127.0.0.1:8080`

## 2) Basic order

1. Create a poll
2. Get one token
3. Ask server to sign your blinded vote
4. Submit final vote with signature
5. Read results

## 3) Endpoints

### Health
`GET /health`

### Create poll
`POST /polls/create`

Example body:
```json
{
  "title": "Demo Poll",
  "candidates": ["Alice", "Bob"],
  "token_count": 1
}
```

### Issue token
`POST /polls/{poll_id}/issue-token`

Example body:
```json
{
  "count": 1
}
```

### Sign blinded message
`POST /polls/{poll_id}/sign`

Example body:
```json
{
  "token": "tok_...",
  "blinded_message": "BLINDED_..."
}
```

### Submit vote
`POST /polls/{poll_id}/submit`

Example body:
```json
{
  "ballot": "Alice|ballot-001",
  "signature": "SIG_..."
}
```

`ballot` format is always:
`Candidate|ballot_id`

### Get results
`GET /polls/{poll_id}/results`

### Close poll
`POST /polls/{poll_id}/close`

## 4) Minimal PowerShell example

```powershell
# Create poll
$create = Invoke-RestMethod -Method Post -Uri 'http://127.0.0.1:8080/polls/create' -ContentType 'application/json' -Body (@{
  title = 'Demo Poll'
  candidates = @('Alice','Bob')
  token_count = 1
} | ConvertTo-Json -Compress)

$pollId = $create.poll_id
$token = $create.initial_tokens[0]

# Sign blinded message
$sign = Invoke-RestMethod -Method Post -Uri "http://127.0.0.1:8080/polls/$pollId/sign" -ContentType 'application/json' -Body (@{
  token = $token
  blinded_message = 'BLINDED_Alice|ballot-001'
} | ConvertTo-Json -Compress)

# Submit vote
Invoke-RestMethod -Method Post -Uri "http://127.0.0.1:8080/polls/$pollId/submit" -ContentType 'application/json' -Body (@{
  ballot = 'Alice|ballot-001'
  signature = 'SIG_STUB'
} | ConvertTo-Json -Compress)

# Results
Invoke-RestMethod -Method Get -Uri "http://127.0.0.1:8080/polls/$pollId/results"
```
