import { ThemeList } from '../../../../../client/settings/themes';
import { useForceUpdate } from '../../../../utils';

import { SettingItem } from './settingsItem';

export const ThemesList = () => {
    const forceUpdate = useForceUpdate();
    return (
        <>
            {ThemeList.map((hotkey, index) => {
                return (
                    <SettingItem
                        setting={hotkey}
                        key={index}
                        isTheme={true}
                        forceUpdate={forceUpdate}
                    />
                );
            })}
        </>
    );
};
