#if EDITOR

#include "TerminalOutputBuffer.h"

#include <utility>

void TerminalOutputBuffer::Append(TerminalEntryKind kind, std::string text, float timestamp)
{
    if (text.empty())
    {
        return;
    }

    TerminalOutputEntry entry;
    entry.mKind = kind;
    entry.mText = std::move(text);
    entry.mTimestamp = timestamp;

    std::lock_guard<std::mutex> lock(mMutex);
    mPending.push_back(std::move(entry));
    mDirty = true;
}

void TerminalOutputBuffer::Drain()
{
    std::deque<TerminalOutputEntry> drained;
    {
        std::lock_guard<std::mutex> lock(mMutex);
        if (mPending.empty())
        {
            mDirty = false;
            return;
        }
        drained.swap(mPending);
        mDirty = false;
    }

    for (TerminalOutputEntry& entry : drained)
    {
        mEntries.push_back(std::move(entry));
    }

    while (mEntries.size() > kMaxEntries)
    {
        mEntries.pop_front();
    }
}

void TerminalOutputBuffer::Clear()
{
    mEntries.clear();
    // Pending entries (if any) remain and will arrive on the next Drain.
}

bool TerminalOutputBuffer::IsDirty() const
{
    std::lock_guard<std::mutex> lock(mMutex);
    return mDirty;
}

#endif
