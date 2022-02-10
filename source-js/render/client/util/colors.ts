import React from 'react';
import { hexToRGB, rgbToHsl } from '../cell/colors';

export const getUIColorStyle = (color = '#FFFFFF') => {
    const [r, g, b] = hexToRGB(color).map(i => i * 255);
    const [_, __, l] = rgbToHsl(r, g, b);

    // const border = /border: (none|#[0-9a-f]{6})/i.exec(color)?.[1];

    const gradient = /gradient: (\d+deg #[0-9a-f]{6} #[0-9a-f]{6})/i.exec(color)?.[1];

    const glow = /glow: (#[0-9a-f]{6})/i.exec(color)?.[1];

    const style: React.CSSProperties = {
        color: `rgb(${~~r},${~~g},${~~b})`,
        backgroundColor: l > 0.25 ? 'transparent' : '#eeeeee',
    };

    if (gradient) {
        style.color = 'transparent';
        style.backgroundImage = `linear-gradient(${gradient.split(' ').join(', ')})`;
        style.WebkitBackgroundClip = 'text';
    }

    return style;
};

export const getBadgeStyleRGB = (color: number[]) => {
    const [r, g, b] = color;
    const [h, s, l] = rgbToHsl(r * 255, g * 255, b * 255);
    return {
        color: l > 0.8 ? '#111111' : '#eeeeee',
        backgroundColor: `rgb(${r * 255},${g * 255},${b * 255})`,
    };
};
