import { createState } from '@hookstate/core';
import DefaultSkin from '../img/default.png';

export const defaultSkin = DefaultSkin;

const InitialState: Inputs = {
    name: '',
    skin1: null,
    skin2: null,
};

interface Inputs {
    name?: string;
    skin1?: string;
    skin2?: string;
}

interface SkinListResponse {
    maxSlots?: number;
    mySkins?: string[];
}

let bootState: Inputs & SkinListResponse;

const DefaultSkinStore = {
    maxSlots: 30,
    mySkins: [],
};

export const SaveInputs = () => {
    const copy = Object.assign({}, InputStore.value);
    Object.assign(copy, SkinStore.value);
    localStorage.setItem('cytos-inputs', JSON.stringify(copy));
};

try {
    bootState = JSON.parse(localStorage.getItem('cytos-inputs') || 'null');
    if (typeof bootState !== 'object') bootState = {};
    if (typeof bootState.name === 'undefined') bootState.name = '';
    if (typeof bootState.skin1 === 'undefined') bootState.skin1 = '';
    if (typeof bootState.skin2 === 'undefined') bootState.skin2 = '';

    if (Array.isArray(bootState.mySkins)) DefaultSkinStore.mySkins = bootState.mySkins;

    delete bootState.mySkins;
    delete bootState.maxSlots;
} catch {
    bootState = InitialState;
}

export const InputStore = createState<Inputs>(bootState);

export const SkinStore = createState<SkinListResponse>(DefaultSkinStore);

SaveInputs();

export const GetInputs = () => {
    return [
        InputStore.name.value || '',
        InputStore.skin1.value || '',
        InputStore.skin2.value || '',
    ];
};
