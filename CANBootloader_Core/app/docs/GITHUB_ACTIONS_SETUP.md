# GitHub Actions Setup Guide for macOS Code Signing

This guide explains how to set up automatic code signing for your macOS application using GitHub Actions.

## Prerequisites

- GitHub repository for your project
- Apple Developer Account
- Access to a Mac computer (one-time setup to export certificates)

## Step 1: Export Your Code Signing Certificate (On a Mac)

1. Open **Keychain Access** on your Mac
2. In the left sidebar, select **My Certificates**
3. Find your "Developer ID Application" certificate
4. Right-click the certificate → **Export**
5. Save as `.p12` file with a strong password
6. Remember this password!

## Step 2: Prepare Certificate for GitHub

On your Mac, convert the certificate to Base64:

```bash
base64 -i YourCertificate.p12 | pbcopy
```

This copies the Base64 string to your clipboard.

## Step 3: Add Secrets to GitHub

1. Go to your GitHub repository
2. Click **Settings** → **Secrets and variables** → **Actions**
3. Click **New repository secret**
4. Add these secrets:

### Required Secrets:

| Secret Name | Value | Description |
|-------------|-------|-------------|
| `MACOS_CERTIFICATE` | [Base64 string from Step 2] | Your exported .p12 certificate |
| `MACOS_CERTIFICATE_PASSWORD` | [Your .p12 password] | Password you used when exporting |
| `KEYCHAIN_PASSWORD` | [Any random password] | Temporary keychain password |
| `SIGNING_IDENTITY` | `Developer ID Application: Your Name (TEAM_ID)` | Your signing identity name |

### Optional Secrets (for Notarization - removes all warnings):

| Secret Name | Value | Description |
|-------------|-------|-------------|
| `ENABLE_NOTARIZATION` | `true` | Enable notarization |
| `APPLE_ID` | your-apple-id@email.com | Your Apple ID |
| `APPLE_TEAM_ID` | YOUR_TEAM_ID | Your 10-character Team ID |
| `APPLE_APP_PASSWORD` | [app-specific password] | Generate at appleid.apple.com |

### To find your Signing Identity:

On your Mac, run:
```bash
security find-identity -v -p codesigning
```

Look for something like: `Developer ID Application: Your Name (ABC1234567)`

### To find your Team ID:

1. Go to https://developer.apple.com/account
2. Click on **Membership** in the sidebar
3. Your Team ID is shown there (10 characters)

### To create an App-Specific Password:

1. Go to https://appleid.apple.com
2. Sign in
3. Go to **Security** → **App-Specific Passwords**
4. Click **Generate an app-specific password**
5. Enter a name like "GitHub Actions"
6. Copy the generated password

## Step 4: Push to GitHub

1. Initialize git (if not already):
```bash
git init
git add .
git commit -m "Add GitHub Actions workflow"
```

2. Create a repository on GitHub.com

3. Push your code:
```bash
git remote add origin https://github.com/your-username/your-repo.git
git branch -M main
git push -u origin main
```

## Step 5: Run the Workflow

### Option A: Manual Trigger
1. Go to your repository on GitHub
2. Click **Actions** tab
3. Click **Build and Sign macOS App**
4. Click **Run workflow** → **Run workflow**

### Option B: Automatic (on push/tag)
Just push code or create a tag:
```bash
git tag v1.0.0
git push origin v1.0.0
```

## Step 6: Download the Signed App

1. Go to **Actions** tab
2. Click on the completed workflow run
3. Scroll to **Artifacts**
4. Download **CANBootloaderConsole-macOS-Signed**

## Notes

- **Without notarization**: Users need to right-click → Open on first launch
- **With notarization**: Users can just double-click (no warnings!)
- Notarization takes 5-10 minutes extra
- GitHub Actions is completely FREE for public repositories
- For private repos: 2000 minutes/month free

## Troubleshooting

### "No signing identity found"
- Check that `SIGNING_IDENTITY` matches exactly what's in your certificate
- Run `security find-identity -v -p codesigning` on Mac to verify

### "Certificate import failed"
- Verify the Base64 string is complete (no line breaks)
- Verify the certificate password is correct

### Notarization fails
- Verify Apple ID and Team ID are correct
- Verify app-specific password is valid
- Check that 2FA is enabled on your Apple ID

## Need Help?

Check the Actions logs on GitHub for detailed error messages.
