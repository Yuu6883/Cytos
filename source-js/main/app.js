const path = require('path');
const electron = require('electron');
const { app, screen, globalShortcut, BrowserWindow } = electron;

const ICON_PATH = path.resolve(__dirname, '..', 'cytos.ico');

// Uncomment to nuke gpu/benchmark
// app.commandLine.appendSwitch('disable-frame-rate-limit');

class CytosMain {
    constructor() {
        this.ensureSingleInstance();
        this.init();
    }

    ensureSingleInstance() {
        // Enable Single Instance
        const lock = app.requestSingleInstanceLock();
        if (!lock) {
            app.quit();
            return;
        } else {
            app.on('second-instance', () => {
                if (this.window) {
                    if (this.window.isMinimized()) this.window.restore();
                    this.window.focus();
                }
            });
        }
    }

    init() {
        app.on('ready', () => this.createMainWindow())
            .on('window-all-closed', () => process.platform !== 'darwin' && app.quit())
            .on('activate', () => !this.window && createMainWindow())
            .on('quit', () => this.quit());
    }

    createMainWindow() {
        console.log('Creating window');

        const { width, height } = screen.getPrimaryDisplay().workAreaSize;

        this.window = new BrowserWindow({
            icon: ICON_PATH,
            // frame: false,
            // transparent: true,
            center: true,
            closable: true,
            minimizable: true,
            maximizable: true,
            show: false,
            resizable: true,
            width: width * 0.8,
            height: height * 0.8,
            backgroundColor: 'rbg(88, 88, 88)',
            webPreferences: {
                nodeIntegration: true,
                nodeIntegrationInWorker: true,
                contextIsolation: false,
                devTools: true,
            },
        });

        this.window.loadFile(path.join(__dirname, '..', 'build', 'index.html'));
        this.window.webContents.once('did-finish-load', () => {
            console.log('Window content loaded');
            this.window.removeMenu();
            this.window.webContents.setZoomFactor(1);
            this.window.show();
            this.window.webContents.openDevTools();
        });

        this.window.on('focus', () => {
            globalShortcut.register('CommandOrControl+R', _ => {});
            globalShortcut.register('CommandOrControl+Shift+R', _ => {});
            globalShortcut.register('F5', _ => {});
            globalShortcut.register('F11', _ =>
                this.window.setFullScreen(!this.window.isFullScreen()),
            );
        });

        this.window.on('blur', () => {
            globalShortcut.unregisterAll();
        });
    }

    quit() {
        console.log('App exiting');
    }
}

module.exports = CytosMain;
