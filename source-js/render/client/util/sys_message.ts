import { ChatType, sysMsg } from '../../stores/hud';

export const SYS = {
    gameConnected: (server: string) => sysMsg(ChatType.NONE, `Connected to $0`, server),
    gameDisconnected: (server: string, reason?: string) =>
        reason
            ? sysMsg(ChatType.NONE, 'Disconnected from $0: $1', server, reason)
            : sysMsg(ChatType.NONE, 'Disconnected from $0', server),
    gameMsg: (msg: string) => sysMsg(ChatType.GAME, msg),
    webglContextLost: () =>
        sysMsg(ChatType.SYSTEM, 'WebGL context lost, reinitializing...'),
    renderError: () =>
        sysMsg(ChatType.SYSTEM, 'Render error happened, please report on GitHub'),
};
