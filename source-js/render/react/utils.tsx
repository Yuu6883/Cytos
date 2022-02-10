import { useState } from 'react';

export const useForceUpdate = () => {
    const set = useState(0)[1];
    return () => set(s => s + 1);
};

export const getCssVar = (name: string) =>
    getComputedStyle(document.querySelector(':root')).getPropertyValue(`--${name}`);

export const setCssVar = (name: string, value: string) =>
    (document.querySelector(':root') as HTMLElement).style.setProperty(
        `--${name}`,
        value,
    );
