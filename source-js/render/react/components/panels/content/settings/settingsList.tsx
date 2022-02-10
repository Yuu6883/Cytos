import {
    ReplayViewerSettingList,
    SettingCategory,
} from '../../../../../client/settings/settings';
import { useForceUpdate } from '../../../../utils';
import { Collapsible } from './collapsible';
import { SettingItem } from './settingsItem';

export const SettingsList = () => {
    const forceUpdate = useForceUpdate();

    return (
        <>
            {Object.entries(SettingCategory).map(([cat, list], i) => (
                <Collapsible key={i} {...{ cat, list, forceUpdate }}>
                    {list.map((hotkey, index) => {
                        return (
                            <SettingItem
                                setting={hotkey}
                                key={index}
                                forceUpdate={forceUpdate}
                            />
                        );
                    })}
                </Collapsible>
            ))}
        </>
    );
};
