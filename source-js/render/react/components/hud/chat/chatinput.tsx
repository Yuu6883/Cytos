import { useEffect, useRef, useState } from 'react';
import { useState as useStore } from '@hookstate/core';
import Client from '../../../../client';
import Style from '../../../css/chat.module.css';
import { addMsg, ChatType, HUDStore } from '../../../../stores/hud';
import { Select } from '../select';

const OPTIONS = [
    { value: 0, label: 'lobby' },
    // { value: 2, label: 'Guild' },
    // 3 is DM
];

export const ChatInput = () => {
    const [showEmoteList, setShowEmoteList] = useState(false);

    const [text, setText] = useState('');
    const input = useRef<HTMLInputElement>();

    const [index, setIndex] = useState(0);
    const [expand, setExpand] = useState(false);

    const chat = useStore(HUDStore.chatInput);

    const onKeyDown = (event: React.KeyboardEvent<HTMLInputElement>) => {
        const { key, nativeEvent, currentTarget: t } = event;

        if (key === 'Enter') {
            chat.set(false);
            const msg = text.trim();
            if (msg) {
                // Add message to local lmao
                addMsg({
                    id: 0,
                    text: msg,
                    time: Date.now(),
                    type: ChatType.NONE,
                });
            }
            setText('');
        }

        nativeEvent.stopPropagation();
        nativeEvent.stopImmediatePropagation();
    };

    useEffect(() => {
        if (chat.value) input.current.focus();
        else if (input.current !== document.activeElement && text.length > 0) {
            return HUDStore.chatInput.set(true);
        }
    }, [chat.value]);

    return (
        <>
            <div
                className={Style.chatInput}
                style={{ display: chat.value ? 'flex' : 'none' }}
            >
                <div
                    style={{ position: 'relative' }}
                    onClick={e => {
                        setExpand(true);
                        e.stopPropagation();
                    }}
                >
                    <Select
                        expand={expand}
                        style={{
                            whiteSpace: 'nowrap',
                            width: '100%',
                            textOverflow: 'ellipsis',
                            overflow: 'hidden',
                        }}
                        setExpand={setExpand}
                        options={OPTIONS}
                        value={`> ${OPTIONS[index].label}`}
                        onChange={opt => {
                            setExpand(false);
                            setIndex(opt.value);
                        }}
                        position="top"
                    />
                </div>
                <input
                    ref={input}
                    placeholder="Type here..."
                    value={text}
                    onChange={({ target }) => setText(target.value)}
                    onKeyDown={onKeyDown}
                ></input>
            </div>
        </>
    );
};
