import { useState as useStore, State, Downgraded } from '@hookstate/core';
import { useRef, useEffect, useState } from 'react';
import { HUDStore, ChatMessage, ChatType } from '../../../../stores/hud';
import { Select } from '../select';
import ChatStyle from '../../../css/chat.module.css';
import EXPGIF from '../../../../img/exp.gif';
import CYTGIF from '../../../../img/cyt.gif';
import { getUIColorStyle } from '../../../../client/util/colors';

const ChatMessage = ({
    data,
    force,
    prefix,
    autoScroll,
    incre,
}: {
    data: ChatMessage;
    autoScroll: boolean;
    force: boolean;
    prefix?: boolean;
    incre: () => void;
}) => {
    const ref = useRef<HTMLDivElement>(null);

    useEffect(() => {
        if (autoScroll && ref.current) ref.current.scrollIntoView(true);
        else incre();
    }, []);

    if (force) ref.current && ref.current.scrollIntoView(true);

    if (data.gain === 'xp') {
        return (
            <div ref={ref} className={ChatStyle.systemMsg}>
                <span
                    style={{
                        fontSize: '16px',
                        fontWeight: 'bold',
                    }}
                >
                    {data.type}:&nbsp;
                </span>
                <span>
                    +{data.amount}{' '}
                    <img src={EXPGIF} width="30" height="30" data-tip="EXP" alt="EXP" />
                </span>
            </div>
        );
    } else if (data.gain === 'coin') {
        return (
            <div ref={ref} className={ChatStyle.systemMsg}>
                <span
                    style={{
                        fontSize: '16px',
                        fontWeight: 'bold',
                    }}
                >
                    {data.type}:&nbsp;
                </span>
                <span>
                    +{data.amount}{' '}
                    <img src={CYTGIF} width="25" height="25" data-tip="CYT" alt="CYT" />
                </span>
            </div>
        );
    } else if (data.system) {
        return (
            <div ref={ref} className={ChatStyle.systemMsg}>
                {data.type !== ChatType.NONE && (
                    <span
                        style={{
                            fontSize: '16px',
                            fontWeight: 'bold',
                        }}
                    >
                        {data.type}:&nbsp;
                    </span>
                )}
                <span>{data.text}</span>
            </div>
        );
    }

    return (
        <div ref={ref} className={ChatStyle.chatMessage} game-id={data.id}>
            <span
                className={ChatStyle.chatMsgName}
                style={getUIColorStyle(data?.color || '#FFFFFF')}
            >
                {data.name}:{' '}
            </span>
            <span className={ChatStyle.chatMsgText}>
                {data.jsx ? data.jsx : data.text}
            </span>
        </div>
    );
};

const ChatContainer = (props: { data: State<ChatMessage[]>; prefix?: boolean }) => {
    const store = useStore(props.data);
    const data = store.attach(Downgraded).get();
    const [atBottom, setAtBottom] = useState(true);
    const [toBottom, setToBottom] = useState(false);
    const [newMessage, setNewMessage] = useState(0);

    const onScroll: React.UIEventHandler<HTMLDivElement> = e => {
        const bottom = e.currentTarget.clientHeight + e.currentTarget.scrollTop;
        const v = e.currentTarget.scrollHeight - bottom < 20;
        if (atBottom != v) setAtBottom(v);
        if (v) {
            setNewMessage(0);
            if (toBottom) setToBottom(false);
        }
    };

    const goToBottom = () => (setToBottom(true), setNewMessage(0));
    const increNewMessage = () => atBottom || setNewMessage(newMessage + 1);
    const buttonContent = newMessage
        ? `${newMessage} new message${newMessage > 1 ? 's' : ''}`
        : 'To Bottom';

    return (
        <>
            <button className={ChatStyle.toBottom} onClick={goToBottom} hidden={atBottom}>
                {buttonContent}
            </button>
            <div className={ChatStyle.chatContainer} onScroll={onScroll}>
                {data.map((chat, index) => (
                    <ChatMessage
                        key={`${chat.time}-${chat.id}`}
                        autoScroll={atBottom}
                        force={toBottom && index === data.length - 1}
                        prefix={props.prefix}
                        data={chat}
                        incre={increNewMessage}
                    />
                ))}
            </div>
        </>
    );
};

const m = HUDStore.messages;
const cat = [m.all, m.lobby, m.system];
const OPTIONS = [
    { value: 0, label: 'Display All' },
    { value: 1, label: 'Lobby' },
    { value: 2, label: 'System' },
];

export const ChatBox = () => {
    const v = useStore(HUDStore.visible).value;
    const index = useStore(HUDStore.displayIndex);
    const [expand, setExpand] = useState(false);

    return v.all && v.chat ? (
        <div className={ChatStyle.chatBox}>
            <div
                className={ChatStyle.chatNav}
                onClick={e => {
                    e.stopPropagation();
                    setExpand(true);
                }}
            >
                <i
                    className="fas fa-arrow-left"
                    onClick={e => {
                        e.stopPropagation();
                        index.set((index.value + cat.length - 1) % cat.length);
                    }}
                ></i>
                <Select
                    expand={expand}
                    setExpand={setExpand}
                    options={OPTIONS}
                    value={OPTIONS[index.value].label}
                    onChange={opt => {
                        setExpand(false);
                        index.set(opt.value);
                    }}
                    position="center"
                />
                <i
                    className="fas fa-arrow-right"
                    onClick={e => {
                        e.stopPropagation();
                        index.set((index.value + 1) % cat.length);
                    }}
                ></i>
            </div>
            <ChatContainer prefix={!index.value} data={cat[index.value]} />
        </div>
    ) : (
        <></>
    );
};
