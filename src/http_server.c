#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "lwip/tcp.h"

#include "http_server.h"

static struct tcp_pcb *server_pcb = NULL;

static int web_n1 = 0;
static int web_n2 = 0;
static char web_op = '+';
static int web_acertos = 0;
static char web_digitado[16] = "";
static char web_msg[120] = "Aguardando inicio";
static char web_evento[20] = "normal";
static int web_premio_ativo = 0;
static int web_tempo_premio = 0;

void web_update_data(
    int n1,
    char operador,
    int n2,
    int acertos_atual,
    const char *digitado_atual,
    const char *msg,
    const char *evento,
    int premio_ativo,
    int tempo_premio
) {
    web_n1 = n1;
    web_n2 = n2;
    web_op = operador;
    web_acertos = acertos_atual;
    web_premio_ativo = premio_ativo;
    web_tempo_premio = tempo_premio;
    snprintf(web_digitado, sizeof(web_digitado), "%s", digitado_atual);
    snprintf(web_msg,     sizeof(web_msg),     "%s", msg);
    snprintf(web_evento,  sizeof(web_evento),  "%s", evento);
}

/* Envia em fatias respeitando o buffer TCP do lwIP */
static void send_response(struct tcp_pcb *tpcb, const char *data) {
    size_t total = strlen(data);
    size_t sent  = 0;
    while (sent < total) {
        u16_t available = tcp_sndbuf(tpcb);
        if (available == 0) { tcp_output(tpcb); break; }
        size_t chunk = total - sent;
        if (chunk > available) chunk = available;
        if (tcp_write(tpcb, data + sent, (u16_t)chunk, TCP_WRITE_FLAG_COPY) != ERR_OK) break;
        sent += chunk;
    }
    tcp_output(tpcb);
}

static err_t http_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (!p) { tcp_close(tpcb); return ERR_OK; }

    tcp_recved(tpcb, p->tot_len);
    char *req = (char *)p->payload;

    if (strstr(req, "GET /status")) {
        char json[700];
        snprintf(json, sizeof(json),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Cache-Control: no-cache\r\n\r\n"
            "{\"n1\":%d,\"op\":\"%c\",\"n2\":%d,\"acertos\":%d,"
            "\"digitado\":\"%s\",\"msg\":\"%s\",\"evento\":\"%s\","
            "\"premio\":%d,\"tempo\":%d}",
            web_n1, web_op, web_n2, web_acertos,
            web_digitado, web_msg, web_evento,
            web_premio_ativo, web_tempo_premio
        );
        send_response(tpcb, json);

    } else {
        /* =============================================================
         * Gamificacao da Matematica com Robotica Educacional
         * U.I Vila Nova - Professor Luiz
         * Interface infantil: limpa, grande, intuitiva (6-10 anos)
         * ============================================================= */
        const char *html =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=utf-8\r\n\r\n"
            "<!DOCTYPE html><html lang='pt-BR'><head>"
            "<meta charset='UTF-8'>"
            "<meta name='viewport' content='width=device-width,initial-scale=1'>"
            "<title>Matematica com Robotica</title>"
            "<link href='https://fonts.googleapis.com/css2?family=Baloo+2:wght@700;800&family=Nunito:wght@600;700&display=swap' rel='stylesheet'>"
            "<style>"
            "*{box-sizing:border-box;margin:0;padding:0}"

            /* Fundo com bolinhas decorativas */
            "body{"
            "font-family:'Nunito',sans-serif;"
            "min-height:100vh;"
            "background:#FFF7ED;"
            "display:flex;flex-direction:column;align-items:center;"
            "justify-content:flex-start;"
            "padding:0 0 24px;"
            "overflow-x:hidden;"
            "}"

            /* Topo colorido - AZUL */
            ".topo{"
            "width:100%;background:#2563EB;"
            "padding:14px 20px 18px;"
            "text-align:center;"
            "position:relative;"
            "}"
            ".topo::after{"
            "content:'';"
            "position:absolute;bottom:-16px;left:0;right:0;height:16px;"
            "background:#2563EB;"
            "clip-path:ellipse(52% 100% at 50% 0%);"
            "}"
            ".projeto-nome{"
            "font-family:'Baloo 2',cursive;"
            "font-size:17px;font-weight:800;"
            "color:#DBEAFE;"
            "letter-spacing:.3px;"
            "line-height:1.2;"
            "}"
            ".escola-nome{"
            "font-size:13px;font-weight:600;"
            "color:rgba(255,255,255,.7);"
            "margin-top:3px;"
            "}"

            /* Estrelas de progresso - maiores */
            ".estrelas-wrap{"
            "margin:44px 0 8px;"
            "display:flex;gap:10px;justify-content:center;"
            "}"
            ".estrela{"
            "font-size:56px;"
            "transition:transform .25s;"
            "filter:grayscale(1) opacity(.3);"
            "}"
            ".estrela.on{"
            "filter:none;"
            "animation:popStar .35s ease;"
            "}"
            "@keyframes popStar{"
            "0%{transform:scale(.5)}60%{transform:scale(1.3)}100%{transform:scale(1)}"
            "}"

            /* Card central - maior */
            ".card{"
            "background:#fff;"
            "border-radius:32px;"
            "padding:40px 32px 32px;"
            "margin:16px 16px 0;"
            "width:calc(100% - 32px);"
            "max-width:560px;"
            "box-shadow:0 8px 0 #BFDBFE,0 10px 30px rgba(0,0,0,.08);"
            "text-align:center;"
            "}"

            /* A conta em si - ENORME para criancas */
            ".conta{"
            "font-family:'Baloo 2',cursive;"
            "font-size:100px;"
            "font-weight:800;"
            "line-height:1;"
            "margin-bottom:20px;"
            "letter-spacing:-2px;"
            "}"
            ".n1{color:#3B82F6;}"
            ".op{color:#8B5CF6;}"
            ".n2{color:#EC4899;}"
            ".eq{color:#94A3B8;font-size:80px;}"
            ".qm{color:#10B981;animation:pulse .9s ease-in-out infinite alternate;}"
            "@keyframes pulse{from{transform:scale(1)}to{transform:scale(1.1)}}"

            /* Separador */
            ".sep{height:2px;background:#F1F5F9;border-radius:2px;margin:0 0 18px;}"

            /* Campo de resposta */
            ".resp-label{"
            "font-size:15px;font-weight:700;color:#94A3B8;"
            "text-transform:uppercase;letter-spacing:2px;margin-bottom:8px;"
            "}"
            ".digitado{"
            "font-family:'Baloo 2',cursive;"
            "font-size:88px;font-weight:800;"
            "color:#1E293B;"
            "min-height:110px;"
            "display:flex;align-items:center;justify-content:center;"
            "background:#F8FAFC;"
            "border-radius:20px;"
            "border:3px solid #E2E8F0;"
            "transition:border-color .2s,box-shadow .2s;"
            "}"
            ".digitado.ativo{"
            "border-color:#3B82F6;"
            "box-shadow:0 0 0 4px rgba(59,130,246,.1);"
            "}"
            ".cursor{"
            "display:inline-block;width:4px;height:72px;"
            "background:#3B82F6;border-radius:2px;margin-left:4px;"
            "animation:blink .7s step-end infinite;"
            "}"
            "@keyframes blink{50%{opacity:0}}"

            /* Mensagem de feedback */
            ".msg{"
            "margin:14px 16px 0;"
            "width:calc(100% - 32px);max-width:560px;"
            "background:#fff;"
            "border-radius:20px;"
            "padding:20px 24px;"
            "text-align:center;"
            "box-shadow:0 4px 0 #BFDBFE,0 6px 20px rgba(0,0,0,.06);"
            "}"
            "#mensagem{"
            "font-size:22px;font-weight:700;color:#374151;"
            "min-height:26px;"
            "}"

            /* Instrucoes do teclado */
            ".teclado-info{"
            "display:flex;gap:8px;justify-content:center;"
            "margin-top:14px;flex-wrap:wrap;"
            "padding:0 16px;"
            "}"
            ".tecla{"
            "background:#fff;border-radius:12px;"
            "padding:6px 12px;"
            "font-size:16px;font-weight:700;color:#64748B;"
            "box-shadow:0 3px 0 #E2E8F0;"
            "}"
            ".tecla b{color:#2563EB;font-family:'Baloo 2',cursive;font-size:18px;}"

            /* Flash de feedback */
            ".flash{"
            "position:fixed;inset:0;"
            "display:flex;align-items:center;justify-content:center;"
            "pointer-events:none;z-index:10;"
            "}"
            ".flash-box{"
            "font-family:'Baloo 2',cursive;"
            "font-size:72px;font-weight:800;"
            "border-radius:36px;padding:20px 40px;"
            "color:#fff;opacity:0;"
            "}"
            ".flash-box.acerto{"
            "background:#10B981;"
            "animation:flashIn .85s ease forwards;"
            "}"
            ".flash-box.erro{"
            "background:#EF4444;"
            "animation:flashShake .85s ease forwards;"
            "}"
            "@keyframes flashIn{"
            "0%{opacity:0;transform:scale(.4)}"
            "35%{opacity:1;transform:scale(1.08)}"
            "75%{opacity:1;transform:scale(1)}"
            "100%{opacity:0;transform:scale(.85)}"
            "}"
            "@keyframes flashShake{"
            "0%{opacity:0;transform:scale(.5)}"
            "20%{opacity:1;transform:scale(1) translateX(-12px)}"
            "40%{transform:translateX(12px)}"
            "60%{transform:translateX(-8px)}"
            "80%{transform:translateX(0)}"
            "100%{opacity:0}"
            "}"

            /* Confetes */
            ".cf{"
            "position:fixed;pointer-events:none;z-index:9;"
            "width:10px;height:14px;border-radius:3px;"
            "animation:cfCai 1.6s ease-in forwards;"
            "}"
            "@keyframes cfCai{"
            "0%{transform:translateY(-10px) rotate(0);opacity:1}"
            "100%{transform:translateY(105vh) rotate(720deg);opacity:0}"
            "}"

            /* Tela de PREMIO - AZUL */
            "#premio{"
            "position:fixed;inset:0;z-index:20;"
            "background:#2563EB;"
            "display:none;flex-direction:column;"
            "align-items:center;justify-content:center;"
            "text-align:center;padding:32px;"
            "}"
            ".premio-trofeu{font-size:130px;animation:trofeuBounce 1s ease infinite alternate;}"
            "@keyframes trofeuBounce{"
            "from{transform:translateY(0) rotate(-5deg)}"
            "to{transform:translateY(-12px) rotate(5deg)}"
            "}"
            ".premio-titulo{"
            "font-family:'Baloo 2',cursive;"
            "font-size:68px;font-weight:800;"
            "color:#fff;"
            "text-shadow:0 4px 0 rgba(0,0,0,.15);"
            "margin:10px 0 6px;"
            "}"
            ".premio-sub{"
            "font-size:16px;font-weight:700;"
            "color:rgba(255,255,255,.85);"
            "max-width:320px;line-height:1.5;"
            "}"
            ".timer-box{"
            "margin-top:24px;"
            "background:rgba(255,255,255,.2);"
            "border-radius:24px;padding:12px 36px;"
            "}"
            ".timer-label{"
            "font-size:13px;font-weight:700;color:rgba(255,255,255,.7);"
            "text-transform:uppercase;letter-spacing:2px;"
            "}"
            "#timer{"
            "font-family:'Baloo 2',cursive;"
            "font-size:100px;font-weight:800;color:#fff;"
            "line-height:1;"
            "}"
            "</style></head><body>"

            /* Topo */
            "<div class='topo'>"
            "<div class='projeto-nome'>Gamifica&ccedil;&atilde;o da Matem&aacute;tica<br>com Rob&oacute;tica Educacional</div>"
            "<div class='escola-nome'>U.I Vila Nova &mdash; Professor Luiz</div>"
            "</div>"

            /* Estrelas */
            "<div class='estrelas-wrap'>"
            "<span class='estrela' id='s1'>&#11088;</span>"
            "<span class='estrela' id='s2'>&#11088;</span>"
            "<span class='estrela' id='s3'>&#11088;</span>"
            "<span class='estrela' id='s4'>&#11088;</span>"
            "<span class='estrela' id='s5'>&#11088;</span>"
            "</div>"

            /* Card principal */
            "<div class='card'>"
            "<div class='conta'>"
            "<span class='n1' id='n1'>?</span>"
            "<span class='op' id='op'> + </span>"
            "<span class='n2' id='n2'>?</span>"
            "<span class='eq'> = </span>"
            "<span class='qm'>?</span>"
            "</div>"
            "<div class='sep'></div>"
            "<div class='resp-label'>sua resposta</div>"
            "<div class='digitado' id='digitado'>"
            "<span id='dig-txt'>-</span>"
            "<span class='cursor'></span>"
            "</div>"
            "</div>"

            /* Mensagem */
            "<div class='msg'><div id='mensagem'>Carregando...</div></div>"

            /* Dicas teclado */
            "<div class='teclado-info'>"
            "<div class='tecla'><b>0-9</b> digitar</div>"
            "<div class='tecla'><b>*</b> apagar</div>"
            "<div class='tecla'><b>#</b> confirmar</div>"
            "</div>"

            /* Flash feedback */
            "<div class='flash'><div class='flash-box' id='flash'></div></div>"

            /* Tela premio */
            "<div id='premio'>"
            "<div class='premio-trofeu'>&#127942;</div>"
            "<div class='premio-titulo'>PARAB&Eacute;NS!</div>"
            "<div class='premio-sub'>Voc&ecirc; resolveu tudo!<br>A caixa est&aacute; aberta!</div>"
            "<div class='timer-box'>"
            "<div class='timer-label'>fechando em</div>"
            "<div id='timer'>20</div>"
            "</div>"
            "</div>"

            "<script>"

            /* Audio sintetizado */
            "var AC=null;"
            "function ctx(){if(!AC)AC=new(window.AudioContext||window.webkitAudioContext)();return AC;}"
            "function beep(f,d,t,v){"
            "var c=ctx(),o=c.createOscillator(),g=c.createGain();"
            "o.connect(g);g.connect(c.destination);"
            "o.type=t||'sine';o.frequency.value=f;"
            "g.gain.setValueAtTime(v||.35,c.currentTime);"
            "g.gain.exponentialRampToValueAtTime(.001,c.currentTime+d);"
            "o.start();o.stop(c.currentTime+d);"
            "}"
            "function somAcerto(){"
            "beep(523,.15);setTimeout(function(){beep(659,.15);},130);"
            "setTimeout(function(){beep(784,.15);},260);"
            "setTimeout(function(){beep(1047,.3);},390);"
            "}"
            "function somErro(){"
            "beep(280,.18,'sawtooth',.3);"
            "setTimeout(function(){beep(200,.28,'sawtooth',.3);},200);"
            "}"
            "function somPremio(){"
            "var ns=[523,659,784,1047,1319];"
            "ns.forEach(function(n,i){setTimeout(function(){beep(n,.2,'sine',.4);},i*120);});"
            "setTimeout(function(){beep(1047,.6,'sine',.4);},700);"
            "}"

            /* Confetes - azul no lugar do laranja */
            "var CFC=['#2563EB','#FFD93D','#10B981','#3B82F6','#8B5CF6','#EC4899','#fff'];"
            "function confetes(n){"
            "for(var i=0;i<n;i++){(function(){"
            "var d=document.createElement('div');"
            "d.className='cf';"
            "d.style.left=(Math.random()*100)+'%';"
            "d.style.background=CFC[Math.floor(Math.random()*CFC.length)];"
            "d.style.animationDelay=(Math.random()*.5)+'s';"
            "d.style.width=(8+Math.random()*8)+'px';"
            "d.style.height=(10+Math.random()*10)+'px';"
            "document.body.appendChild(d);"
            "setTimeout(function(){d.remove();},2000);"
            "})();}"
            "}"

            /* Flash de feedback */
            "function flash(txt,tipo){"
            "var f=document.getElementById('flash');"
            "f.className='flash-box';"
            "void f.offsetWidth;"
            "f.textContent=txt;"
            "f.className='flash-box '+tipo;"
            "}"

            /* Estrelas */
            "var ultAcertos=-1;"
            "function atualizaEstrelas(n){"
            "for(var i=1;i<=5;i++){"
            "var el=document.getElementById('s'+i);"
            "if(i<=n){el.textContent='\\u2B50';el.classList.add('on');}else{el.textContent='\\u2B50';el.classList.remove('on');}"
            "}"
            "}"

            /* Loop principal */
            "var ultEvento='normal';"
            "function atualizar(){"
            "fetch('/status').then(function(r){return r.json();}).then(function(d){"

            /* Conta - converte operador para simbolo visual */
            "var opS=d.op==='x'?'\u00d7':d.op==='-'?'\u2212':d.op==='/'?'\u00f7':d.op;"
            "document.getElementById('n1').textContent=d.n1;"
            "document.getElementById('op').textContent=' '+opS+' ';"
            "document.getElementById('n2').textContent=d.n2;"

            /* Resposta digitada */
            "var dig=d.digitado||'';"
            "document.getElementById('dig-txt').textContent=dig||'-';"
            "var el=document.getElementById('digitado');"
            "el.className='digitado'+(dig?' ativo':'');"

            /* Mensagem */
            "document.getElementById('mensagem').textContent=d.msg;"

            /* Estrelas */
            "if(d.acertos!==ultAcertos){atualizaEstrelas(d.acertos);ultAcertos=d.acertos;}"

            /* Eventos */
            "if(d.evento!==ultEvento){"
            "if(d.evento==='acerto'){flash('\\u2713 Certo!','acerto');confetes(28);somAcerto();}"
            "if(d.evento==='erro'){flash('Ops! Tente de novo','erro');somErro();}"
            "if(d.evento==='premio'){confetes(70);somPremio();}"
            "ultEvento=d.evento;"
            "}"

            /* Premio */
            "var p=document.getElementById('premio');"
            "if(d.premio==1){"
            "p.style.display='flex';"
            "document.getElementById('timer').textContent=d.tempo;"
            "confetes(4);"
            "}else{p.style.display='none';}"

            "}).catch(function(){});}"
            "setInterval(atualizar,400);atualizar();"
            "</script></body></html>";

        send_response(tpcb, html);
    }

    pbuf_free(p);
    tcp_close(tpcb);
    return ERR_OK;
}

static err_t http_accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, http_recv_callback);
    return ERR_OK;
}

void http_server_init(void) {
    server_pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!server_pcb) { printf("Erro ao criar servidor HTTP\n"); return; }
    if (tcp_bind(server_pcb, IP_ANY_TYPE, 80) != ERR_OK) { printf("Erro ao abrir porta 80\n"); return; }
    server_pcb = tcp_listen(server_pcb);
    tcp_accept(server_pcb, http_accept_callback);
    printf("Servidor HTTP ativo em http://192.168.4.1\n");
}