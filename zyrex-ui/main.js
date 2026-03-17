const { app, BrowserWindow, ipcMain } = require('electron')
const path = require('path')
const { spawn } = require('child_process')
const http = require('http')

let mainWindow
let pythonProcess

// ─────────────────────────────────────────────────────────────
//  START PYTHON API SERVER
// ─────────────────────────────────────────────────────────────
function startPythonAPI() {
  const pythonPath = path.join(__dirname, '..', 'python_nlp', 'zyrex')
  const scriptPath = path.join(pythonPath, 'zyrex_api.py')

  console.log('Starting Python API:', scriptPath)

  pythonProcess = spawn('python', [scriptPath], {
    cwd: pythonPath,
    stdio: ['pipe', 'pipe', 'pipe']
  })

  pythonProcess.stdout.on('data', (data) => {
    console.log('[Python]', data.toString().trim())
  })

  pythonProcess.stderr.on('data', (data) => {
    console.error('[Python ERR]', data.toString().trim())
  })

  pythonProcess.on('close', (code) => {
    console.log('[Python] Process exited with code', code)
  })
}

// ─────────────────────────────────────────────────────────────
//  WAIT FOR API TO BE READY
// ─────────────────────────────────────────────────────────────
function waitForAPI(retries = 20) {
  return new Promise((resolve, reject) => {
    let attempts = 0
    const check = () => {
      http.get('http://localhost:5000/status', (res) => {
        resolve()
      }).on('error', () => {
        attempts++
        if (attempts >= retries) {
          console.log('API not responding — opening UI anyway')
          resolve()
        } else {
          setTimeout(check, 500)
        }
      })
    }
    check()
  })
}

// ─────────────────────────────────────────────────────────────
//  CREATE WINDOW
// ─────────────────────────────────────────────────────────────
async function createWindow() {
  // Start Python in background
  startPythonAPI()

  // Wait up to 10 seconds for Python API
  await waitForAPI()

  mainWindow = new BrowserWindow({
    width: 1400,
    height: 860,
    minWidth: 1100,
    minHeight: 700,
    frame: false,          // custom titlebar
    transparent: false,
    backgroundColor: '#080c14',
    webPreferences: {
      nodeIntegration: false,
      contextIsolation: true,
      preload: path.join(__dirname, 'preload.js')
    },
    icon: path.join(__dirname, 'assets', 'icon.png'),
    show: false            // show after ready-to-show
  })

  mainWindow.loadFile(path.join(__dirname, 'src', 'index.html'))

  mainWindow.once('ready-to-show', () => {
    mainWindow.show()
  })

  // Open DevTools in development
  // mainWindow.webContents.openDevTools()
}

// ─────────────────────────────────────────────────────────────
//  IPC HANDLERS  (UI → Main process)
// ─────────────────────────────────────────────────────────────
ipcMain.on('window-minimize', () => mainWindow.minimize())
ipcMain.on('window-maximize', () => {
  if (mainWindow.isMaximized()) mainWindow.unmaximize()
  else mainWindow.maximize()
})
ipcMain.on('window-close', () => {
  if (pythonProcess) pythonProcess.kill()
  app.quit()
})

// ─────────────────────────────────────────────────────────────
//  APP EVENTS
// ─────────────────────────────────────────────────────────────
app.whenReady().then(createWindow)

app.on('window-all-closed', () => {
  if (pythonProcess) pythonProcess.kill()
  if (process.platform !== 'darwin') app.quit()
})

app.on('before-quit', () => {
  if (pythonProcess) pythonProcess.kill()
})
