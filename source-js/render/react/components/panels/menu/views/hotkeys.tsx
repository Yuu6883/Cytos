import { KeybindsItems } from '../../content/hotkeys/keybinds/keybindList';
import { MouseList } from '../../content/hotkeys/mouse/mouseList';
import { Collapsible } from '../../content/settings/collapsible';
import Style from '../../../../css/hotkeys.module.css';

export const Hotkeys = () => {
    return (
        <div className={Style.list}>
            <KeybindsItems />
            <Collapsible cat="Mouse">
                <MouseList />
            </Collapsible>
        </div>
    );
};
