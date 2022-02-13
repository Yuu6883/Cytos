import { useEffect, useRef, useState } from 'react';
import ReactToolTip from 'react-tooltip';
import Style from '../../../../css/skins.module.css';
import { useState as useStore, none } from '@hookstate/core';
import {
    defaultSkin,
    InputStore,
    SaveInputs,
    SkinStore,
} from '../../../../../stores/inputs';
import { MouseSVG } from '../../content/icons/mouse';
import Client from '../../../../../client';
import { Themes } from '../../../../../client/settings/themes';
import { hexToRGB } from '../../../../../client/cell/colors';
import { getBadgeStyleRGB } from '../../../../../client/util/colors';
import { basicPopup } from './popups';

export const Skins = () => {
    const inputs = useStore(InputStore);
    const skins = useStore(SkinStore);

    // Bad lib
    useEffect(() => void ReactToolTip.hide(), []);

    useEffect(() => {
        ReactToolTip.rebuild();
    }, [skins.value]);

    const onClickItem = (index: number, type: number) => {
        const selected = skins.mySkins.value[index];
        if (type === 1)
            inputs.skin1.set(inputs.skin1.value === selected ? null : selected);
        if (type === 2)
            inputs.skin2.set(inputs.skin2.value === selected ? null : selected);
        SaveInputs();
    };

    const onBlur = () => SaveInputs();

    const onPaste: React.ClipboardEventHandler<HTMLInputElement> = e => {
        const url = e.clipboardData.getData('text');
        if (skins.mySkins.value.includes(url)) {
            basicPopup(`Duplicate skin ignored: "${url}"`);
            return;
        }
        const img = new Image();
        img.onload = () => {
            skins.mySkins.merge([url]);
        };
        img.onerror = () => basicPopup(`Failed to load image from "${url}"`);
        img.src = url;
    };

    const onDelete = async (e: React.MouseEvent<HTMLElement, MouseEvent>, index) => {
        e.stopPropagation();
        e.preventDefault();
        skins.mySkins.merge({ [index]: none });
        ReactToolTip.hide();
    };

    const canUpload = skins.mySkins.length < skins.maxSlots.value;

    return (
        <div className={Style.container}>
            <div style={{ display: 'inline-flex', padding: '4px', alignItems: 'center' }}>
                <input
                    onBlur={onBlur}
                    onPaste={onPaste}
                    value={''}
                    onChange={e => e.currentTarget.value}
                    placeholder={canUpload ? 'Paste URL Here' : 'All slots are used'}
                    disabled={!canUpload}
                />
            </div>
            <p className={Style.instr}>
                <MouseSVG />
                &nbsp;&nbsp;select skin 1&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
                <MouseSVG />
                &nbsp;&nbsp;select skin 2
            </p>
            <div className={Style.skins}>
                {Array.from({ length: skins.maxSlots.value }, (_, i) => {
                    const skin = skins.mySkins.value[i + 2];

                    if (i === 0) {
                        return (
                            <Skin
                                key={0}
                                url=""
                                onClick={() => {
                                    inputs.skin1.set('');
                                    SaveInputs();
                                }}
                                onContextMenu={() => {
                                    inputs.skin2.set('');
                                    SaveInputs();
                                }}
                                isSkin1={inputs.skin1.value === ''}
                                isSkin2={inputs.skin2.value === ''}
                                empty
                            />
                        );
                    } else if (i === 1) {
                        return (
                            <Skin
                                key={1}
                                url={defaultSkin}
                                onClick={() => {
                                    inputs.skin1.set(defaultSkin);
                                    SaveInputs();
                                }}
                                onContextMenu={() => {
                                    inputs.skin2.set(defaultSkin);
                                    SaveInputs();
                                }}
                                isSkin1={inputs.skin1.value === defaultSkin}
                                isSkin2={inputs.skin2.value === defaultSkin}
                            />
                        );
                    } else if (skin) {
                        return (
                            <Skin
                                key={skin + i}
                                url={skin}
                                onClick={() => onClickItem(i, 1)}
                                onContextMenu={() => onClickItem(i, 2)}
                                isSkin1={inputs.skin1.value === skin}
                                isSkin2={inputs.skin2.value === skin}
                                onDelete={e => onDelete(e, i)}
                            />
                        );
                    } else {
                        return <EmptySlot key={i} tip="Empty Slot" />;
                    }
                })}
            </div>
        </div>
    );
};

type SkinProps = {
    url: string;
    onClick: () => void;
    onContextMenu: () => void;
    onDelete?: (e: React.MouseEvent<HTMLElement, MouseEvent>) => void;
    isSkin1: boolean;
    isSkin2: boolean;
    empty?: boolean;
};

const getBadge = (isPrim: boolean, isSec: boolean) => {
    if (isPrim && isSec) return 1;
    if (isPrim) return 2;
    if (isSec) return 3;
    else return null;
};

const badgeNames = ['', 'Skin 1 + 2', 'Skin 1', 'Skin 2'];

const EmptySlot = (props: { tip: string }) => (
    <div className={Style.empty} data-tip={props.tip}></div>
);

const Skin = ({
    url,
    onClick,
    onContextMenu,
    onDelete,
    isSkin1,
    isSkin2,
    empty,
}: SkinProps) => {
    const [color, setColor] = useState([1, 1, 1]);
    const [isEmpty, setEmpty] = useState(!!empty);

    const badge = getBadge(isSkin1, isSkin2);

    let border = '';
    if (Themes.autoTheme.v) {
        if (isSkin1) border = `rgb(${color.map(v => ~~(v * 255)).join(',')})`;
        else if (isSkin2) border = Themes.inactiveTabBorder.v;
    } else {
        if (isSkin1) border = Themes.activeTabBorder.v;
        else if (isSkin2) border = Themes.inactiveTabBorder.v;
    }

    const badgeColors = [
        '',
        Themes.activeTabBorder.v,
        Themes.activeTabBorder.v,
        Themes.inactiveTabBorder.v,
    ];

    return (
        <div {...{ onClick, onContextMenu }} className={Style.skin}>
            {badge && (
                <div className={Style.badge}>
                    <span
                        style={getBadgeStyleRGB(
                            Themes.autoTheme.v
                                ? color
                                : [...hexToRGB(badgeColors[badge])],
                        )}
                    >
                        {badgeNames[badge]}
                    </span>
                </div>
            )}
            {onDelete && <i className="fas fa-times" onClick={onDelete} />}
            {isEmpty ? (
                <EmptySlot tip="Empty Skin" />
            ) : (
                <img
                    style={{ border: `3px solid ${border}` }}
                    className="rainbow"
                    crossOrigin="anonymous"
                    src={url}
                    onLoad={e => setColor(Client.sampleColor(e.currentTarget, '#888888'))}
                    onError={_ => setEmpty(true)}
                    alt="Skin"
                />
            )}
        </div>
    );
};
