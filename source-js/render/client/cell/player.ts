import Client from '..';
import { AtlasTexture } from './textures';

export class BasePlayer {
    pid: number;
    isBot: boolean;

    constructor(pid: number, isBot: boolean) {
        this.pid = pid;
        this.isBot = isBot;
    }
}

export class Bot extends BasePlayer {
    constructor(pid: number) {
        super(pid, true);
    }
}

export class Player extends BasePlayer {
    name: string;
    skin: string;
    skinTex: AtlasTexture;
    nameTex: AtlasTexture;
    skinState = -1;
    color: string;

    constructor(pid: number, name: string, skin: string, color: string) {
        super(pid, false);
        this.name = name;
        this.skin = skin;
        this.color = color;

        Client.instance.updateNameTexture(this);
        Client.instance.updateSkinTexture(this);
        // console.log(`new Player#${pid}`, { name, skin, color });
    }

    destroy() {
        Client.instance.nameStore.remove(this.nameTex.key);
        Client.instance.skinStore.remove(this.skinTex.key);
    }

    update(name: string, skin: string, color: string) {
        name = Client.instance.filterLongString(name);

        if (this.color !== color || this.name != name) {
            // console.log(`Name: ${this.name}(${oldColor}) -> ${name}(${color})`);
            Client.instance.nameStore.remove(this.nameTex.key);

            this.name = name;
            this.color = color;

            Client.instance.updateNameTexture(this);
        }

        if (this.skin !== skin) {
            // console.log(`Skin: ${this.skin} -> ${skin}`);
            Client.instance.skinStore.remove(this.skinTex.key);

            this.skin = skin;

            Client.instance.updateSkinTexture(this);
        }
    }
}
