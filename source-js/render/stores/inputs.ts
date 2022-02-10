import { createState } from '@hookstate/core';
import DefaultSkin from '../img/default.png';

export const defaultSkin = DefaultSkin;

const InitialState: Inputs = {
    name: '',
    skin1: null,
    skin2: null,
    hasInput: false,
    upload: [],
};

interface Inputs {
    name?: string;
    skin1?: string;
    skin2?: string;
    hasInput: boolean;
    upload: string[];
}

interface SkinRecord {
    id: number;
    hash: string;
    size: number;
    label?: string;
    create_at: number;
}

interface SkinListResponse {
    maxSlots: number;
    mySkins: SkinRecord[];
}

let bootState: Inputs & { skins?: string[] };

export const SaveInputs = () => {
    const copy = Object.assign({}, InputStore.value);
    delete copy.upload;
    delete copy.hasInput;
    localStorage.setItem('cytos-inputs', JSON.stringify(copy));
};

try {
    bootState = JSON.parse(localStorage.getItem('cytos-inputs') || 'null');
    if (typeof bootState !== 'object') bootState = { upload: [], hasInput: false };
    if (typeof bootState.name === 'undefined') bootState.name = '';
    if (typeof bootState.skin1 === 'undefined') bootState.skin1 = '';
    if (typeof bootState.skin2 === 'undefined') bootState.skin2 = '';
} catch {
    bootState = InitialState;
}

export const InputStore = createState<Inputs>(bootState);

const DefaultSkinStore = {
    maxSlots: 30,
    mySkins: [],
};

export const SkinStore = createState<SkinListResponse>(DefaultSkinStore);

SaveInputs();

export const GetInputs = () => {
    return [
        InputStore.name.value || '',
        InputStore.skin1.value || '',
        InputStore.skin2.value || '',
    ];
};
