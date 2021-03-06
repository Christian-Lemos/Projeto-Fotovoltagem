let utils = require('./utils')
let $ = require('jquery')
$(document).ready(function()
{
    $(".btn-debug-adicionar-sonoff").on('click', function()
    {
        $.ajax({
            url : '/debug/adicionarsonoff',
            method : 'GET',
            dataType : 'JSON',
            success : function(resposta)
            {
                utils.GerarNotificacao(resposta.mensagem.conteudo, resposta.mensagem.tipo);    
            },
            error : function(a)
            {
                $("html").html(a.responseText);
            }
        })
    });
    $(".btn-debug-adicionar-painelsolar").on('click', function()
    {
        $.ajax({
            url : '/debug/adicionarsolargetter',
            method : 'GET',
            dataType : 'JSON',
            success : function(resposta)
            {
                utils.GerarNotificacao(resposta.mensagem.conteudo, resposta.mensagem.tipo);
                
            },
            error : function(a)
            {
                $("html").html(a.responseText);
            }
        })
    }); 
    $("#debug-form-enviar-mensagem").on('submit', function()
    {
        
        var info = $(this).serialize();
        $.ajax({
            url : '/debug/enviarMensagem',
            method : 'POST',
            data :info,
            dataType : 'JSON',
            success : function(resposta)
            {
                utils.GerarNotificacao(resposta.mensagem.conteudo, resposta.mensagem.tipo);
                if(typeof(AtualizarDispositivos) != 'undefined')
                {
                    AtualizarDispositivos();
                }    
            },
            error : function(a)
            {
                $("html").html(a.responseText);
            }
        })
    }); 
});

