
#include "WorldPacket.h"
#include "WorldSession.h"
#include "WorldSocket.h"
#include "Opcodes.h"

WorldSocket::WorldSocket(SocketHandler &h, WorldSession *s) : TcpSocket(h)
{
    _session = s;
    _gothdr = false;
    _ok=false;
}

bool WorldSocket::IsOk(void)
{
    return _ok;
}

void WorldSocket::OnConnect()
{
    log("Connected to world server.");
    _ok = true;
}

void WorldSocket::OnConnectFailed()
{
    logerror("Connecting to World Server failed!");
    _ok = false;
}

void WorldSocket::OnDelete()
{
    log("Connection to world server has been closed.");
    _ok = false;
    _session->SetMustDie(); // recreate the session if needed
}

void WorldSocket::OnException()
{
    if(_ok)
    {
        logerror("WorldSocket::OnException()");
        _ok = false;
    }
}

void WorldSocket::OnRead()
{
    TcpSocket::OnRead();
    if(!ibuf.GetLength())
    {
        this->CloseAndDelete();
        return;
    }
    while(ibuf.GetLength() > 0) // when all packets from the current ibuf are transformed into WorldPackets the remaining len will be zero
    {

        if(_gothdr) // already got header, this packet has to be the data part
        {
            ASSERT(_remaining > 0); // case pktsize==0 is handled below
            if(ibuf.GetLength() < _remaining)
            {
                DEBUG(logdebug("Delaying WorldPacket generation, bufsize is %u but should be >= %u",ibuf.GetLength(),_remaining));
                break;
            }
            _gothdr=false;
            WorldPacket *wp = new WorldPacket(_remaining);
            wp->resize(_remaining);
            ibuf.Read((char*)wp->contents(),_remaining);
            wp->SetOpcode(_opcode);
            GetSession()->AddToPktQueue(wp);
        }
        else // no pending header stored, so this packet must be a header
        {
            if(ibuf.GetLength() < sizeof(ServerPktHeader))
            {
                DEBUG(logdebug("Delaying header reading, bufsize is %u but should be >= %u",ibuf.GetLength(),sizeof(ServerPktHeader)));
                break;
            }
            ServerPktHeader hdr;
            ibuf.Read((char*)&hdr,sizeof(ServerPktHeader));
            _crypt.DecryptRecv((uint8*)&hdr,sizeof(ServerPktHeader));
            _remaining = ntohs(hdr.size)-2;
            _opcode = hdr.cmd;
            if(_opcode > MAX_OPCODE_ID) // no opcode has yet a number over 1000
            {
                logcritical("CRYPT ERROR: opcode=%u, remain=%u",_opcode,_remaining); // this should never be the case!
                GetSession()->GetInstance()->SetError(); // no way to recover the crypt, must exit
                // if the crypt gets messy its hardly possible to recover it, especially if we dont know
                // the lentgh of the following data part
                // TODO: invent some way how to recover the crypt (reconnect?)
                return;
            }

            // the header is fine, now check if there are more data
            if(_remaining == 0) // this is a packet with no data (like CMSG_NULL_ACTION)
            {
                WorldPacket *wp = new WorldPacket;
                wp->SetOpcode(_opcode);
                GetSession()->AddToPktQueue(wp);
            }
            else // there is a data part to fetch
            {
                _gothdr=true; // only got the header, next packet will contain the data
            }
        }
    }
}

void WorldSocket::SendWorldPacket(WorldPacket &pkt)
{
    if(!_ok)
        return;
    ClientPktHeader hdr;
    memset(&hdr,0,sizeof(ClientPktHeader));
    hdr.size = ntohs(pkt.size()+4);
    hdr.cmd = pkt.GetOpcode();
    _crypt.EncryptSend((uint8*)&hdr, 6);
    ByteBuffer final(pkt.size()+6);
    final.append((uint8*)&hdr,sizeof(ClientPktHeader));
    if(pkt.size())
        final.append(pkt.contents(),pkt.size());
    SendBuf((char*)final.contents(),final.size());
}

void WorldSocket::InitCrypt(uint8 *key,uint32 len)
{
    _crypt.SetKey(key,len);
    _crypt.Init();
}
