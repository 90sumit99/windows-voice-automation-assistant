// ZYREX App v6 — Grok-style black + orange

const API = (window.zyrex?.apiBase) || 'http://localhost:5000'

// ── Particles ─────────────────────────────────────────────────
;(function() {
  const c = document.getElementById('particle-canvas')
  if (!c) return
  const ctx = c.getContext('2d')
  let W, H, pts = []
  const rsz = () => { W = c.width = innerWidth; H = c.height = innerHeight }
  rsz(); addEventListener('resize', rsz)
  for (let i = 0; i < 50; i++) pts.push({
    x: Math.random()*W, y: Math.random()*H,
    vx: (Math.random()-.5)*.18, vy: (Math.random()-.5)*.18,
    r: Math.random()*.9+.3,
    a: Math.random()*.25+.08,
    c: ['#ff6b2b','#ff8c42','#ffb347','#ff6b2b88'][Math.floor(Math.random()*4)]
  })
  const draw = () => {
    ctx.clearRect(0,0,W,H)
    pts.forEach(p => {
      p.x+=p.vx; p.y+=p.vy
      if(p.x<0)p.x=W; if(p.x>W)p.x=0
      if(p.y<0)p.y=H; if(p.y>H)p.y=0
      ctx.beginPath(); ctx.arc(p.x,p.y,p.r,0,Math.PI*2)
      ctx.fillStyle=p.c; ctx.globalAlpha=p.a; ctx.fill()
    })
    for(let i=0;i<pts.length;i++) for(let j=i+1;j<pts.length;j++) {
      const d = Math.hypot(pts[i].x-pts[j].x, pts[i].y-pts[j].y)
      if(d < 100) {
        ctx.beginPath(); ctx.moveTo(pts[i].x,pts[i].y); ctx.lineTo(pts[j].x,pts[j].y)
        ctx.strokeStyle='#ff6b2b'; ctx.globalAlpha=(1-d/100)*.055; ctx.lineWidth=.5; ctx.stroke()
      }
    }
    ctx.globalAlpha=1; requestAnimationFrame(draw)
  }
  draw()
})()

// ── API ───────────────────────────────────────────────────────
const get  = async () => { try { return await (await fetch(`${API}/state`)).json() } catch { return null } }
const post = async cmd => { try { await fetch(`${API}/command`,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({command:cmd})}) } catch{} }

// ── Maps ──────────────────────────────────────────────────────
const SC = { IDLE:'#ff6b2b', LISTENING:'#22c55e', PROCESSING:'#f59e0b', EXECUTING:'#ff8c42', ERROR:'#ef4444', OFFLINE:'#525252' }
const MC = { ONLINE:'#22c55e', OFFLINE:'#ef4444', READY:'#ff6b2b', CONNECTED:'#22c55e', DISCONNECTED:'#ef4444', ACTIVE:'#f59e0b' }
const LC = { INFO:'#38bdf8', SUCCESS:'#22c55e', WARNING:'#f59e0b', ERROR:'#ef4444', COMMAND:'#ff6b2b' }
const MI = { 'Voice Engine':'🎤','AI Brain':'🧠','Audio Controller':'🔊','File System':'📁','App Launcher':'🚀','Policy Engine':'🛡️','Screenshot':'📸','Music Controller':'🎵','C++ Bridge':'⚡' }

const FT = [
  { i:'👁️', n:'Screen Vision',  d:'GPT-4V · See & understand screen' },
  { i:'🖱️', n:'Auto Control',   d:'Mouse & keyboard automation' },
  { i:'🌐', n:'Web Automation', d:'Browser task automation' },
  { i:'🔌', n:'Plugin System',  d:'Extensible command modules' },
  { i:'📱', n:'Mobile Control', d:'Remote from your phone' },
]

const { useState, useEffect, useRef } = React
const h = React.createElement

// ── Waveform ──────────────────────────────────────────────────
const Waveform = ({ bars, status }) => {
  const d   = (bars?.length ? bars : Array(32).fill(.02)).slice(0,32)
  const col = SC[status]||'#ff6b2b'
  return h('div',{className:'wave'},
    d.map((v,i) => h('div',{key:i,className:'wb',style:{
      height: `${Math.max(2,Math.round(v*30))}px`,
      width:  '3px',
      background: `linear-gradient(to top, ${col}cc, ${col}33)`,
      opacity: .4+v*.6
    }}))
  )
}

// ── Stat row ──────────────────────────────────────────────────
const StatRow = ({ icon, label, value, color }) => {
  const pct = Math.max(0,Math.min(100,value||0))
  const col = pct>85?'#ef4444':pct>65?'#f59e0b':color
  return h('div',{className:'st-row'},
    h('span',{className:'st-ico'},icon),
    h('span',{className:'st-lbl'},label),
    h('div',{className:'st-bg'},
      h('div',{className:'st-bar',style:{width:`${pct}%`,background:`linear-gradient(90deg,${col}55,${col})`}})
    ),
    h('span',{className:'st-pct',style:{color:col}},`${Math.round(pct)}%`)
  )
}

// ── Module row ────────────────────────────────────────────────
const ModRow = ({ name, status }) => {
  const col = MC[status]||'#525252'
  return h('div',{className:'mod-row'},
    h('span',{className:'mod-ic'},MI[name]||'⚙'),
    h('div',{className:'mod-inf'},
      h('div',{className:'mod-nm'},name),
      h('div',{className:'mod-st',style:{color:col}},status)
    ),
    h('div',{className:'mod-dot',style:{background:col,boxShadow:`0 0 8px ${col}88`}})
  )
}

// ── Timeline item ─────────────────────────────────────────────
const TlItem = ({ item, index, last }) =>
  h('div',{className:'tl-it'},
    h('div',{className:'tl-axis'},
      h('div',{className:'tl-node'},index+1),
      !last && h('div',{className:'tl-line'}),
      !last && h('div',{className:'tl-arrow'},'▾')
    ),
    h('div',{className:'tl-card'},
      h('div',{className:'tl-ct'},
        h('span',{className:'tl-ci'},item.i),
        h('span',{className:'tl-cn'},item.n),
        h('span',{className:'tl-cs'},'SOON')
      ),
      h('div',{className:'tl-cd'},item.d)
    )
  )

// ── Log row ───────────────────────────────────────────────────
const LogRow = ({ e }) => {
  const col = LC[e.level]||'#94a3b8'
  return h('div',{className:'log-r'},
    h('span',{className:'log-ts'},'['+e.timestamp+']'),
    h('span',{className:'log-lv',style:{color:col}},'['+(e.level||'').padEnd(7)+']'),
    h('span',{className:'log-mg'},e.message)
  )
}

// ── Hide/Show button ──────────────────────────────────────────
const HsBtn = ({ open, onToggle }) =>
  h('button',{
    className: `hs-btn ${open?'visible':'hidden'}`,
    onClick: onToggle
  },
    h('span',{className:'hs-icon'}, open ? '◀' : '▶'),
    open ? 'HIDE' : 'SHOW'
  )

// ── Main ──────────────────────────────────────────────────────
function App() {
  const [s,setS]     = useState({ status:'OFFLINE', current_command:'', modules:{}, log_entries:[], cpu:0, ram:0, disk:0, volume_level:50, battery:100, battery_plugged:true, wifi:false, waveform:Array(32).fill(.02) })
  const [inp,setInp] = useState('')
  const [now,setNow] = useState(new Date())
  const [lp,setLp]   = useState(true)
  const [rp,setRp]   = useState(true)
  const [lb,setLb]   = useState(true)
  const logEl        = useRef(null)

  useEffect(()=>{ const t=setInterval(async()=>{ const d=await get(); if(d)setS(d) },200); return()=>clearInterval(t) },[])
  useEffect(()=>{ const t=setInterval(()=>setNow(new Date()),1000); return()=>clearInterval(t) },[])
  useEffect(()=>{ if(logEl.current) logEl.current.scrollTop=logEl.current.scrollHeight },[s.log_entries?.length])

  const st  = s.status||'OFFLINE'
  const col = SC[st]||'#525252'
  const send = () => { if(inp.trim()){ post(inp.trim()); setInp('') } }
  const toggleVoice = async () => { try { const r = await fetch(`${API}/voice-toggle`,{method:'POST'}); const d = await r.json(); state_info = d.voice } catch(e){} }

  const fT = now.toLocaleTimeString('en-US',{hour12:false,hour:'2-digit',minute:'2-digit',second:'2-digit'})
  const fD = now.toLocaleDateString('en-US',{weekday:'short',month:'short',day:'2-digit',year:'numeric'})

  return h('div',{id:'app'},

    /* ── TITLEBAR ── */
    h('div',{id:'titlebar',className:'card'},
      h('div',{className:'logo'},
        h('div',{className:'logo-mark'},'⚡'),
        h('div',{className:'logo-text'},
          h('div',{className:'logo-name'},
            h('span',null,'ZY'),
            'REX'
          ),
          h('div',{className:'logo-tag'},'AI WINDOWS ASSISTANT')
        )
      ),
      h('div',{className:'tb-gap'}),
      h('div',{className:'status-pill',style:{borderColor:`${col}44`,background:`${col}0e`}},
        h('div',{className:'status-pill-dot',style:{background:col,boxShadow:`0 0 8px ${col}`}}),
        h('div',{className:'status-pill-text',style:{color:col}},`ACTIVE: ${st}`)
      ),
      h('div',{className:'tb-gap'}),
      h('div',{className:'wc'},
        h('button',{className:'wc-btn',onClick:()=>window.zyrex?.minimize()},'─'),
        h('button',{className:'wc-btn',onClick:()=>window.zyrex?.maximize()},'□'),
        h('button',{className:'wc-btn cls',onClick:()=>window.zyrex?.close()},'✕')
      )
    ),

    /* ── BODY ── */
    h('div',{id:'body'},

      /* LEFT */
      h('div',{id:'left',className:`card ${lp?'open':'shut'}`},
        h('div',{className:'ph'},
          h('span',{className:'ph-label',style:{color:'#ff6b2b'}},'System Status'),
          h('div',{className:'ph-gap'}),
          h(HsBtn,{open:lp,onToggle:()=>setLp(o=>!o)})
        ),
        lp && h('div',{className:'pb'},
          h('div',{className:'clock-wrap'},
            h('div',{className:'clock-val'},fT),
            h('div',{className:'clock-date'},fD)
          ),
          h(StatRow,{icon:'⚙',label:'CPU',   value:s.cpu,          color:'#ff6b2b'}),
          h(StatRow,{icon:'▣',label:'RAM',   value:s.ram,          color:'#ff8c42'}),
          h(StatRow,{icon:'◫',label:'DISK',  value:s.disk,         color:'#ffb347'}),
          h(StatRow,{icon:'♪',label:'VOL',   value:s.volume_level, color:'#f59e0b'}),
          h(StatRow,{icon:'⚡',label:'BAT',  value:s.battery,      color:'#eab308'}),
          h('div',{className:'rule'}),
          h('div',{className:'info-r'},
            h('span',{className:'info-l'},'⊛  Network'),
            h('span',{className:'chip',style: s.wifi
              ?{color:'#22c55e',borderColor:'#22c55e55',background:'#22c55e0e'}
              :{color:'#ef4444',borderColor:'#ef444455',background:'#ef44440e'}
            }, s.wifi?'ONLINE':'OFFLINE')
          ),
          h('div',{style:{marginTop:'auto'},className:'info-r'},
            h('span',{style:{color:'#ff6b2b',fontSize:'11px'}},'⬡'),
            h('span',{style:{fontSize:'9px',color:'#525252',marginLeft:'6px',fontWeight:600,letterSpacing:'1px'}},'ZYREX v1.0')
          )
        )
      ),

      /* CENTER */
      h('div',{id:'center',className:'card'},
        h('div',{className:'ph'},
          h('span',{className:'ph-label',style:{color:'#ff6b2b'}},'Command Hub'),
          h('div',{className:'ph-gap'})
        ),
        h('div',{className:'cx-body'},
          h('div',{className:'wordmark'},
            h('div',{className:'wordmark-title'},
              h('span',{className:'hi'},'ZY'),'REX'
            ),
            h('div',{className:'wordmark-sub'},'AI COMMAND HUB')
          ),
          h('div',{className:'rule',style:{width:'100%'}}),

          /* MIC */
          h('div',{
            className:'mic-wrap',
            onClick:()=>{ if(st==='OFFLINE') alert('Build VoiceAutomationCore.exe first.') }
          },
            h('div',{className:'mic-orbit mic-orbit-1'}),
            h('div',{className:'mic-orbit mic-orbit-2'}),
            h('div',{className:'mic-orbit mic-orbit-3'}),
            h('div',{
              className:`mic-btn ${st.toLowerCase()}`,
              style:{borderColor:col,boxShadow:`0 0 26px ${col}33`}
            },'🎤')
          ),

          h('div',{className:'mic-status',style:{color:col}},st),
          h(Waveform,{bars:s.waveform,status:st}),
          h('div',{className:'wake-pill'},'Wake Word: ',h('strong',null,'"Hey Zyrex"')),

          h('button',{
            className: (st==='LISTENING'||st==='IDLE'||st==='EXECUTING'||st==='PROCESSING') ? 'voice-btn on' : 'voice-btn off',
            onClick: async () => {
              try { await fetch(API+'/voice-toggle', {method:'POST'}) } catch(e) {}
            }
          },
            h('div', {className:'vb-dot'}),
            st==='OFFLINE'    ? 'VOICE OFFLINE'    :
            st==='LISTENING'  ? 'LISTENING...'  :
            st==='PROCESSING' ? 'PROCESSING...' :
            st==='EXECUTING'  ? 'EXECUTING...'  : 'VOICE ACTIVE'
          ),

          s.current_command && h('div',{className:'last-box'},
            h('div',{className:'last-box-label'},'LAST COMMAND'),
            h('div',{className:'last-box-text'},s.current_command)
          ),

          h('div',{style:{flex:1}}),

          h('div',{className:'inp-wrap'},
            h('input',{
              className:'inp',
              placeholder:'Type a command or speak...',
              value:inp,
              onChange:e=>setInp(e.target.value),
              onKeyDown:e=>e.key==='Enter'&&send()
            }),
            h('button',{className:'inp-send',onClick:send},'➤')
          )
        )
      ),

      /* RIGHT */
      h('div',{id:'right',className:`card ${rp?'open':'shut'}`},
        h('div',{className:'ph'},
          h('span',{className:'ph-label',style:{color:'#ff6b2b'}},'Module Status'),
          h('div',{className:'ph-gap'}),
          h(HsBtn,{open:rp,onToggle:()=>setRp(o=>!o)})
        ),
        rp && h('div',{className:'pb'},
          ...Object.entries(s.modules||{}).map(([n,st])=>h(ModRow,{key:n,name:n,status:st})),
          h('div',{className:'rule'}),
          h('div',{className:'tl-head'},
            h('span',{className:'tl-head-t'},'FUTURE FEATURES'),
            h('span',{className:'tl-head-b'},'ROADMAP')
          ),
          h('div',{className:'tl'},
            ...FT.map((f,i)=>h(TlItem,{key:i,item:f,index:i,last:i===FT.length-1}))
          )
        )
      )
    ),

    /* ── LOG BAR ── */
    h('div',{id:'logbar',className:`card ${lb?'open':'shut'}`},
      h('div',{className:'log-head'},
        h('div',{className:'pulse-dot'}),
        h('span',{className:'log-title'},'LIVE TASK FEED'),
        h('div',{className:'live-badge'},h('div',{className:'pulse-dot'}),'LIVE'),
        h('div',{style:{marginLeft:'auto',display:'flex',gap:'8px',alignItems:'center'}},
          h('button',{className:'log-clr-btn',onClick:()=>setS(s=>({...s,log_entries:[]}))}, 'CLEAR'),
          h(HsBtn,{open:lb,onToggle:()=>setLb(o=>!o)})
        )
      ),
      lb && h('div',{className:'log-body',ref:logEl},
        ...(s.log_entries||[]).slice(-80).map((e,i)=>h(LogRow,{key:i,e}))
      )
    )
  )
}

ReactDOM.createRoot(document.getElementById('root')).render(h(App))
