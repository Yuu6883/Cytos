const path = require('path');
const electron = require('electron');
const { app, BrowserWindow, ipcMain, Tray, Menu } = electron;

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
                if (this.mainWindow) {
                    if (this.mainWindow.isMinimized()) this.mainWindow.restore();
                    this.mainWindow.focus();
                }
            });
        }
    }

    init() {
        app.on('ready', () => this.createMainWindow())
            .on('window-all-closed', () => process.platform !== 'darwin' && app.quit())
            .on('activate', () => !this.mainWindow && createMainWindow())
            .on('before-quit', () => this.beforeQuit())
            .on('quit', () => this.onQuit());

        ipcMain
            .on('minimize', () => this.mainWindow && this.mainWindow.hide())
            .on('toggle', () => this.toggle())
            .on('close', () => this.closeMain())
            .on('quit', () => this.quit())
            .on('restart', () => (app.relaunch(), app.exit(0)));
    }

    createMainWindow() {
        this.mainWindow = new BrowserWindow({
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

        this.mainWindow.loadFile(path.join(__dirname, '..', 'build', 'index.html'));

        this.tray = new Tray(ICON_PATH);
        this.tray.setToolTip('Cytos');
        this.tray.setContextMenu(
            Menu.buildFromTemplate([
                { label: 'Relaunch', click: () => (app.relaunch(), app.exit(0)) },
                { label: 'Exit', click: () => app.exit(0) },
            ]),
        );

        this.tray.on('double-click', () => {
            this.mainWindow.show();
        });

        this.mainWindow.on('ready-to-show', () => {
            // this.mainWindow.removeMenu();
            this.mainWindow.webContents.setZoomFactor(1);
            this.mainWindow.show();
        });

        this.mainWindow.webContents.on('dom-ready', () => {
            this.mainWindow.webContents.send('appPath', app.getAppPath());
            this.mainWindow.webContents.send('downloadPath', app.getPath('downloads'));
        });

        this.mainWindow.on('close', e => this.alertQuit(e));
    }

    quit() {
        console.log('App exiting');

        this.tray.destroy();
        this.mainWindow.destroy();
        app.quit(0);
        process.exit(0);
    }

    /** @param {Electron.Event} e */
    alertQuit(e) {
        if (!this.mainWindow) return;
        this.mainWindow.show();
        this.mainWindow.focus();
        this.mainWindow.center();
        this.mainWindow.webContents.send('alertQuit');

        e.preventDefault();
    }

    toggle() {
        if (!this.mainWindow) return;

        this.mainWindow.isMaximized()
            ? this.mainWindow.unmaximize()
            : this.mainWindow.maximize();
        this.mainWindow.movable = !this.mainWindow.isMaximized();
    }

    showMain() {
        this.mainWindow.flashFrame(true);
        setTimeout(() => this.mainWindow.flashFrame(false), 3000);
        this.mainWindow.show();
        this.mainWindow.restore();
        this.mainWindow.focus();
    }

    closeMain() {
        this.mainWindow && this.mainWindow.hide();
    }

    beforeQuit() {}

    onQuit() {
        this.tray && !this.tray.isDestroyed() && this.tray.destroy();
    }
}

module.exports = CytosMain;
