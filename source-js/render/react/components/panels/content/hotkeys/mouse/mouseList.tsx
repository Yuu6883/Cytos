import { MiceList } from '../../../../../../client/settings/mouse';
import { MouseItem } from './mouseItem';

export const MouseList = () => {
    return (
        <>
            {MiceList.map((hotkey, index) => {
                return <MouseItem hotkey={hotkey} key={index} />;
            })}
        </>
    );
};
