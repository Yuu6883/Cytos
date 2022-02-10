import Style from '../../css/panel.module.css';

interface Props {
    children: React.ReactNode;
    className?: string;
    style?: React.CSSProperties;
    transition?: string;
}

export const Panel = (props: Props) => (
    <div
        className={`${Style.panel} ${props.className || ''}`}
        style={props.style}
        panel-transition={props.transition || ''}
    >
        {props.children}
    </div>
);
