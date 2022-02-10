import { S, Settings } from '../../../../../client/settings/settings';
import { Themes, updateCursor } from '../../../../../client/settings/themes';
import { Setting } from '../../../../../client/settings/setting';

import Style from '../../../../css/settings.module.css';
import Client from '../../../../../client';
import { HUDStore } from '../../../../../stores/hud';
import { Hotkeys } from '../../../../../client/settings/keybinds';

interface Props {
    setting: Setting<boolean | number | string>;
    isTheme?: boolean;
    isHotkey?: boolean;
    forceUpdate: Function;
}

const findSetting = (
    parent: { [_: string]: S<boolean | string | number> },
    k: string,
) => {
    return Object.values(parent).find(ref => ref.k === k)?.v;
};

export const SettingItem = (props: Props) => {
    const { setting, forceUpdate } = props;

    const parent = props.isTheme ? Themes : props.isHotkey ? Hotkeys : Settings;
    const disabled = !setting.dep.every(s =>
        s[0] === '!' ? !findSetting(parent, s.slice(1)) : findSetting(parent, s),
    );

    return (
        <div className={`${Style.item} ${disabled ? Style.disabled : ''}`}>
            <span>
                {setting.valueMap[setting.value as string] || setting.details.text}
            </span>
            {setting.type === 0 ? (
                <>
                    <input
                        id={setting.ref.k}
                        type="checkbox"
                        onChange={e => {
                            setting.v = e.target.checked;

                            if (!Client.instance) return;
                            if (setting.ref.k === 'showHUD')
                                HUDStore.visible.all.set(e.target.checked);
                            if (setting.ref.k === 'resolution') {
                                setTimeout(() => Client.instance.resize(), 500); // hmm.
                            }
                            if (setting.ref.k === 'cursor') updateCursor();
                            forceUpdate();
                        }}
                        checked={setting.value as boolean}
                    />
                    <div className={Style.checkbox}>
                        <label htmlFor={setting.ref.k}></label>
                    </div>
                </>
            ) : setting.type === 1 ? (
                <>
                    <input
                        style={{
                            width: `${
                                ((setting.details.max - setting.details.min + 1) /
                                    setting.details.step) *
                                1.5
                            }em`,
                            margin: 'auto',
                            float: 'right',
                        }}
                        id={setting.ref.k}
                        type="range"
                        min={setting.details.min}
                        max={setting.details.max}
                        step={setting.details.step}
                        onChange={({ target }) => {
                            setting.v = target.valueAsNumber as any;
                            forceUpdate();
                        }}
                        value={Number(setting.ref.v)}
                    />
                </>
            ) : setting.type === 2 ? (
                <>
                    <input
                        id={setting.ref.k}
                        type="color"
                        style={{
                            margin: 'auto',
                            height: '20px',
                            float: 'right',
                            outline: 'none',
                        }}
                        onChange={({ target }) => {
                            parent[setting.ref.k] = setting.v = target.value;
                            forceUpdate();
                        }}
                        value={setting.value as string}
                    />
                </>
            ) : (
                <>
                    <span className={Style.value}>
                        {setting.details.multi
                            ? (Number(setting.value) * setting.details.multi).toFixed(
                                  setting.details.precision || 2,
                              )
                            : Number(setting.value)}
                        {setting.details.unit ? ` ${setting.details.unit}` : ''}
                    </span>
                    <input
                        id={setting.ref.k}
                        type="range"
                        min={setting.details.min}
                        max={setting.details.max}
                        step={setting.details.step}
                        onChange={({ target }) => {
                            setting.v = target.valueAsNumber as any;
                            forceUpdate();
                        }}
                        value={Number(setting.value)}
                    />
                </>
            )}
        </div>
    );
};
