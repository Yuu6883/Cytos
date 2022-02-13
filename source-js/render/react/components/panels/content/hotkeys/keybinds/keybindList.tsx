import { useForceUpdate } from '../../../../../utils';

import { HotkeyHandler } from '../../../../../../client/settings/hotkey-handler';
import { Setting } from '../../../../../../client/settings/setting';
import {
    Hotkeys,
    HotkeysList,
    HotkeyCategory,
} from '../../../../../../client/settings/keybinds';

import Style from '../../../../../css/hotkeys.module.css';
import { SettingItem } from '../../settings/settingsItem';
import { Collapsible } from '../../settings/collapsible';

interface ItemProps {
    forceUpdate: () => void;
    setting: Setting<string>;
}

export const KeyItem = (props: ItemProps) => {
    const { forceUpdate, setting } = props;
    const showAsMinion = !!(Hotkeys.minionMode.v && setting.details.minion);

    const onKeyDown =
        (minion: boolean) => (event: React.KeyboardEvent<HTMLInputElement>) => {
            event.stopPropagation();
            event.preventDefault();

            const ele = event.nativeEvent;
            ele.preventDefault();

            let key = HotkeyHandler.getKeyString(ele);
            if (!key) return;

            HotkeysList.forEach(hotkey => {
                if (hotkey !== setting && hotkey.value === key) {
                    hotkey.v = '';
                }
                if (hotkey.minionValue === key) {
                    hotkey.setMinionValue = '';
                }
            });

            if (key === 'DELETE' || key === 'BACKSPACE') key = '';

            minion ? (setting.setMinionValue = key) : (setting.v = key);

            event.currentTarget.blur();
            forceUpdate();
        };

    return (
        <div className={Style.item} key={setting.ref.k}>
            <span>{setting.details.text}</span>
            {showAsMinion && (
                <input
                    readOnly
                    value={(setting.minionValue as string) || ''}
                    onKeyDown={onKeyDown(true)}
                />
            )}
            <input
                readOnly
                value={(setting.value as string) || ''}
                onKeyDown={onKeyDown(false)}
            />
        </div>
    );
};

const Item = ({
    setting,
    forceUpdate,
}: {
    forceUpdate: () => void;
    setting: Setting<string | boolean | number>;
}) => {
    if (setting.type === 5) {
        return (
            <KeyItem
                {...{
                    forceUpdate,
                    setting: setting as Setting<string>,
                }}
            />
        );
    } else {
        return (
            <SettingItem
                {...{
                    forceUpdate,
                    setting,
                    isHotkey: true,
                }}
            />
        );
    }
};

export const KeybindsItems = () => {
    const forceUpdate = useForceUpdate();

    return (
        <>
            {Object.entries(HotkeyCategory).map(([cat, list], i) =>
                list.length > 1 ? (
                    <Collapsible key={i} cat={cat}>
                        {list.map(setting => (
                            <Item key={setting.ref.k} {...{ setting, forceUpdate }} />
                        ))}
                    </Collapsible>
                ) : (
                    <Item key={i} {...{ setting: list[0], forceUpdate }} />
                ),
            )}
        </>
    );
};
