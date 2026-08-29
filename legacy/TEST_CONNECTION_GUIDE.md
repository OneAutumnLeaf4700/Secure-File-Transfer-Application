# Testing Your Secure File Transfer Application

## Method 1: Public Test SSH Servers

Here are some publicly available SSH test servers you can use:

### Option A: test.rebex.net (Recommended)
- **Server**: test.rebex.net
- **Port**: 22
- **Username**: demo
- **Password**: password
- **Protocol**: SSH/SFTP
- **Note**: This is a read-only test server

### Option B: demo.wftpserver.com
- **Server**: demo.wftpserver.com
- **Port**: 2222
- **Username**: demo-user
- **Password**: demo-user
- **Protocol**: SFTP

## Testing Steps:

1. **Launch your application**:
   ```
   .\x64\Debug\Secure File Transfer Application.exe
   ```

2. **Fill in connection details**:
   - Server: `test.rebex.net`
   - Port: `22`
   - Username: `demo`
   - Password: `password`

3. **Click Connect** and observe:
   - Status bar messages
   - Connection success/failure dialog
   - Button state changes

## Expected Results:

### ✅ Success Indicators:
- Status shows "Connecting..." then "Connected"
- Success dialog: "Successfully connected to server!"
- Connect button becomes disabled
- Disconnect button becomes enabled

### ❌ Failure Indicators:
- Error dialog with specific error message:
  - "Authentication failed" - Wrong credentials
  - "Cannot reach host" - Network/server issue
  - "Connection timed out" - Server not responding
  - "Network error" - General connection problem

## Alternative: Local Testing with WSL

If you have WSL (Windows Subsystem for Linux), you can set up a local SSH server:

1. Install WSL Ubuntu
2. In WSL: `sudo apt update && sudo apt install openssh-server`
3. Start SSH: `sudo service ssh start`
4. Test with: `localhost`, port `22`, your WSL username/password

## Troubleshooting:

### Connection Issues:
- Check internet connection
- Try different test servers
- Verify port numbers
- Check Windows Firewall

### Application Issues:
- Check console output for detailed errors
- Verify all DLLs are in the output directory
- Ensure x64 build was used

## Network Layer Validation:

Your NetworkLayer should handle:
- [x] Socket creation and connection
- [x] SSH handshake
- [x] Authentication
- [x] SFTP session initialization
- [x] Error reporting
- [x] Clean disconnect
