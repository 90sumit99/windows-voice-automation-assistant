const { contextBridge, ipcRenderer } = require('electron')

// Expose safe APIs to the renderer (React)
contextBridge.exposeInMainWorld('zyrex', {
  // Window controls
  minimize: () => ipcRenderer.send('window-minimize'),
  maximize: () => ipcRenderer.send('window-maximize'),
  close:    () => ipcRenderer.send('window-close'),

  // API base URL
  apiBase: 'http://localhost:5000'
})
