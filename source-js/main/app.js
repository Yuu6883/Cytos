const path = require('path');
const electron = require('electron');
const { app, ipcMain, globalShortcut, BrowserWindow } = electron;

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
            .on('before-quit', () => this.beforeQuit())
            .on('quit', () => this.onQuit());

        ipcMain
            .on('minimize', () => this.window && this.window.hide())
            .on('toggle', () => this.toggle())
            .on('close', () => this.closeMain())
            .on('quit', () => this.quit())
            .on('restart', () => (app.relaunch(), app.exit(0)));
    }

    createMainWindow() {
        console.log('Creating window');

        this.window = new BrowserWindow({
            icon: ICON_PATH,
            // frame: false,
            // transparent: true,
            center: true,
            closable: true,
            minimizable: false,
            maximizable: false,
            show: false,
            resizable: true,
            minWidth: 1280,
            minHeight: 720,
            width: 1280,
            height: 720,
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
            // this.mainWindow.removeMenu();
            this.window.webContents.setZoomFactor(1);
            this.window.show();
        });

        this.window.on('close', e => this.alertQuit(e));
        this.window.on('focus', () => {
            globalShortcut.register('CommandOrControl+R', _ => {});
            globalShortcut.register('CommandOrControl+Shift+R', _ => {});
            globalShortcut.register('F5', _ => {});
        });

        this.window.on('blur', () => {
            globalShortcut.unregisterAll();
        });
    }

    quit() {
        console.log('App exiting');

        this.window.destroy();
        app.quit(0);
        process.exit(0);
    }

    /** @param {Electron.Event} e */
    alertQuit(e) {
        if (!this.window) return;
        this.window.show();
        this.window.focus();
        this.window.center();
        this.window.webContents.send('alertQuit');

        e.preventDefault();
    }

    toggle() {
        if (!this.window) return;

        this.window.isMaximized() ? this.window.unmaximize() : this.window.maximize();
        this.window.movable = !this.window.isMaximized();
    }

    showMain() {
        this.window.flashFrame(true);
        setTimeout(() => this.window.flashFrame(false), 3000);
        this.window.show();
        this.window.restore();
        this.window.focus();
    }

    closeMain() {
        this.window && this.window.hide();
    }

    beforeQuit() {}

    onQuit() {}
}

module.exports = CytosMain;
